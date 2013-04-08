#include <netinet/in.h>    // for sockaddr_in
#include <sys/types.h>    // for socket
#include <sys/socket.h>    // for socket
#include <stdio.h>        // for printf
#include <stdlib.h>        // for exit
#include <string.h>        // for bzero
#include <stdint.h>        
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>
#include <string>
#include "ascheck.h"
#include "tools.h"
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
*/


class Benchmark {
	public:
		Benchmark() {}

		void addbench(int numbers,const std::string &part) {
			struct timeval tvstart, tvend;
			int du = 0;
			RandomGenerator gen;
			std::string key,ret,value;

			value = "1";
			int rc=0;
			printf("add bench .... \n");
			gettimeofday(&tvstart, NULL);	
			for (int i=0;i<numbers;i++)
			{

				gen.RandomString(&(gen.rand),16,&key);
				rc = ascheck::add(part,key,value);
				assert(rc>=0);
				if (rc == 1)
					du++;
				
				if (i%10000 == 0)
					printf("\tdeal %d\n",i);
			}

			gettimeofday(&tvend, NULL);	
			int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
			if (total==0) total = 1;
			printf("deal %d use %d(ms),du = %d\n",numbers,total,du);
			printf("%d transaction per second\n",numbers*1000/total);
			printf("add bench .... end .... \n\n");


		}

		void delbench(int numbers,const std::string &part) {
			struct timeval tvstart, tvend;
			RandomGenerator gen;
			std::string key,ret,value;

			int rc=0;
			printf("del bench .... \n");
			gettimeofday(&tvstart, NULL);	
			for (int i=0;i<numbers;i++)
			{

				gen.RandomString(&(gen.rand),16,&key);
				rc = ascheck::del(part,key);
				assert(rc==0);
				
				if (i%10000 == 0)
					printf("\tdeal %d\n",i);
			}

			gettimeofday(&tvend, NULL);	
			int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
			if (total==0) total = 1;
			printf("deal %d use %d(ms)\n",numbers,total);
			printf("%d transaction per second\n",numbers*1000/total);
			printf("del bench .... end .... \n\n");


		}

		void querybench(int numbers,const std::string &part) {
			struct timeval tvstart, tvend;
			RandomGenerator gen;
			std::string key,ret,value;

			int rc=0;
			printf("query bench .... \n");
			std::string v;
			gettimeofday(&tvstart, NULL);	
			for (int i=0;i<numbers;i++)
			{

				gen.RandomString(&(gen.rand),16,&key);
				rc = ascheck::query(part,key,v);
				assert(rc>=0);
				
				if (i%10000 == 0)
					printf("\tdeal %d\n",i);
			}

			gettimeofday(&tvend, NULL);	
			int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
			if (total==0) total = 1;
			printf("deal %d use %d(ms)\n",numbers,total);
			printf("%d transaction per second\n",numbers*1000/total);
			printf("query bench .... end .... \n\n");

		}


		void test(const std::string &part) {
			struct timeval tvstart, tvend;
			RandomGenerator gen;
			std::string key,ret,other,value;
			int rc=0;
			gen.RandomString(&(gen.rand),16,&key);
			gen.RandomString(&(gen.rand),16,&value);
			gen.RandomString(&(gen.rand),16,&other);
			rc = ascheck::del(part,key);
			assert(rc == 0);

			printf("begin test ---- \n");
			printf("\tdel key,not exist ==> Ok\n");

			rc = ascheck::add(part,key,value);
			assert(rc == 0);

			printf("\tadd key ==> Ok\n");

			rc = ascheck::add(part,key,value);
			assert(rc == 1);
			printf("\tadd dup key ==> Ok\n");

			string v;
			rc = ascheck::query(part,key,v);
			assert(rc == 0);
			assert(v == value);
			printf("\tquery key ==> Ok\n");

			rc = ascheck::query(part,key+other,v);
			assert(rc == 1);
			printf("\tquery key ,not exist ==> Ok\n");


			rc = ascheck::del(part,key);
			assert(rc == 0);
			printf("\tdel key ==> Ok\n");
			printf("end test ---- \n");
		}


};

int main(int argc, char **argv)
{
	std::string part = "test_level_db";
	std::string type = "undefine";
	int c,count = 1;
	 

	while ( (c = getopt(argc, argv, "c:n:t:")) != -1) {
		switch (c) {
			case 'c':
				count= atoi(optarg);
				break;
			case 'n':
				part = optarg;
				break;
			case 't':
				type = optarg;
				break;
		}
	}


	Benchmark b;
	if (type == "test")
		b.test(part);
	else if (type == "addbench")
		b.addbench(count,part);
	else if (type == "querybench")
		b.querybench(count,part);
	else if (type == "delbench")
		b.delbench(count,part);
	else if (type == "all") {
		b.addbench(count,part);
		b.querybench(count,part);
		b.delbench(count,part);
	} else {
		printf("invalid arg :\n  test -t all|test|addbench|querybench|delbench [-c 1000] [-n dbname] \n");
	}

}

