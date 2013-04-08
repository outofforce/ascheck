#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tools.h>

extern "C" {
#include <hiredis/hiredis.h>
}

int main(void) {

	redisContext *c;
	redisReply *reply;
	int numbers = 50000;

	struct timeval tvstart, tvend;
	struct timeval timeout = { 1, 500000 }; // 1.5 seconds

	c = redisConnectWithTimeout((char*)"127.0.0.2", 6379, timeout);
	if (c->err) {
		printf("Connection error: %s\n", c->errstr);
		exit(1);
	}

	RandomGenerator gen;
	std::string key,ret;

	int du = 0;
	gettimeofday(&tvstart, NULL);	
	for (int i=0;i<numbers;i++)
	{
		gen.RandomString(&(gen.rand),16,&key);
		//sprintf(command_buf,"GET %s",key.c_str());
		reply = (redisReply *)redisCommand(c,"GET %s",key.c_str());
		if (reply->str == NULL) {
			freeReplyObject(reply);
			reply = (redisReply *)redisCommand(c,"SET %s %s", key.c_str(), "1");
		} else {
			du++;
		}
		freeReplyObject(reply);

		if (i%10000 == 0)
			printf("deal %d\n",i);
		//printf("==>%s\n",ret.c_str());
	}


	gettimeofday(&tvend, NULL);	
	int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
	printf("%d use %d(ms),du = %d\n",numbers,total,du);
	printf("%d transaction per second\n",numbers*1000/total);


}

int test(void) {
    unsigned int j;
    redisContext *c;
    redisReply *reply;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout((char*)"127.0.0.2", 6379, timeout);
    if (c->err) {
        printf("Connection error: %s\n", c->errstr);
        exit(1);
    }

    /* PING server */
    reply = (redisReply *)redisCommand(c,"PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key */
    reply = (redisReply *)redisCommand(c,"SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = (redisReply *)redisCommand(c,"SET %b %b", "bar", 3, "hello", 5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = (redisReply *)redisCommand(c,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(c,"GET f88");
    printf("GET f88: %s\n", reply->str);
    freeReplyObject(reply);


    reply = (redisReply *)redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = (redisReply *)redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = (redisReply *)redisCommand(c,"DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        snprintf(buf,64,"%d",j);
        reply = (redisReply *)redisCommand(c,"LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }

    /* Let's check what we have inside the list */
    reply = (redisReply *)redisCommand(c,"LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);

    return 0;
}
