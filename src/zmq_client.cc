#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "tools.h"
using namespace std;




int main(int argc, char *argv[])
{

	char c;
	int index  = 0;


	while ( (c = getopt(argc, argv, "n:")) != -1) {
		switch (c) {
			case 'n':
				index = atoi(optarg);
				break;
		}
	}
	printf("index = %d\n",index);

	void *ctx = zmq_init (1);
	assert (ctx);

	void *pb = zmq_socket (ctx, ZMQ_PULL);
	assert (pb);


	string head = makeToken(index);
	int rc = zmq_bind(pb, (std::string("ipc:///tmp/fanglf_to_server_")+head).c_str());
	//int rc = zmq_connect(sb, "tcp://127.0.0.1:3000");
	assert (rc == 0);

	void *sb = zmq_socket (ctx, ZMQ_PUSH);
	assert (sb);
	rc = zmq_connect (sb, "ipc:///tmp/fanglf_zero_ipc");
	assert (rc == 0);
  
	struct timeval tvstart, tvend;
	int numbers = 100000;
	int du = 0;
	RandomGenerator gen;
	std::string key,ret;

	gettimeofday(&tvstart, NULL);	
	for (int i=0;i<numbers;i++)
	{
		if (i%10000 == 0)
			printf("deal %d\n",i);
		gen.RandomString(&(gen.rand),16,&key);

		my_msg_send(sb,head+key);
		my_msg_recv(pb,ret);
		ret = ret.substr(8,ret.length()-8);
		if (ret[0] != '0')
			du++;
	}


	gettimeofday(&tvend, NULL);	
	int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
	printf("%d use %d(ms),du = %d\n",numbers,total,du);
	printf("%d transaction per second\n",numbers*1000/total);




	rc = zmq_close (sb);
	assert (rc == 0);
	rc = zmq_close (pb);
	assert (rc == 0);


	rc = zmq_term (ctx);
	assert (rc == 0);

	return 0;
}

