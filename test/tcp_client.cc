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
/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
*/


class Random {
	private:
		uint32_t seed_;
	public:
		explicit Random(uint32_t s) : seed_(s & 0x7fffffffu) { }
		uint32_t Next() {
			static const uint32_t M = 2147483647L;   // 2^31-1
			static const uint64_t A = 16807;  // bits 14, 8, 7, 5, 2, 1, 0
			uint64_t product = seed_ * A;

			seed_ = static_cast<uint32_t>((product >> 31) + (product & M));
			if (seed_ > M) {
				seed_ -= M;
			}
			return seed_;
		}
		uint32_t Uniform(int n) { return Next() % n; }

		bool OneIn(int n) { return (Next() % n) == 0; }

		uint32_t Skewed(int max_log) {
			return Uniform(1 << Uniform(max_log + 1));
		}
};


class RandomGenerator {
	public:

		std::string data_;
		int pos_;
		Random rand;      


		RandomGenerator() :rand(1000) {
			Random rnd(301);
			std::string piece;
			while (data_.size() < 1048576) {
				RandomString(&rnd, 100, &piece);
				data_.append(piece);
			}
			pos_ = 0;
		}

		void RandomString(Random* rnd, int len, std::string* dst) {
			dst->resize(len);
			for (int i = 0; i < len; i++) {
				(*dst)[i] = static_cast<char>(' ' + rnd->Uniform(95));  
			}
		}	

		std::string Generate(int len) {
			if (pos_ + len > data_.size()) {
				pos_ = 0;
				assert(len < data_.size());
			}
			pos_ += len;
			return std::string(data_.data() + pos_ - len, len);
		}
};


const int BUFFER_SIZE = 512;
class do_db_query {

	public:
		int client_socket;
		char buffer[BUFFER_SIZE];
		bool inited;
		do_db_query():inited(false) {}

		~do_db_query() {

			if (inited)
				close(client_socket);

		}


		int pingpong(const std::string &sbuf,std::string &rbuf) {

			int ret = send(client_socket,sbuf.c_str(),sbuf.length(),0);
			if (ret < 0) {
				printf("Send Data Error!");
				return 6;
			}

			bzero(buffer,BUFFER_SIZE);
			int length = recv(client_socket,buffer,BUFFER_SIZE,0);
			if(length < 0)
			{
				printf("Recieve Data Error!");
				return 7;
			} 
			else 
			{
				buffer[length]='\0';
				rbuf = buffer;
			}

			return 0;

		}


		int init(const std::string &addr,int port) {

			struct sockaddr_in client_addr;
			bzero(&client_addr,sizeof(client_addr)); 
			client_addr.sin_family = AF_INET;   
			client_addr.sin_addr.s_addr = htons(INADDR_ANY);
			client_addr.sin_port = htons(0);   
			client_socket = socket(AF_INET,SOCK_STREAM,0);
			if( client_socket < 0)
			{
				printf("Create Socket Failed!\n");
				return 1;
			}
			if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
			{
				printf("Client Bind Port Failed!\n"); 
				return 2;
			}

			struct sockaddr_in server_addr;
			bzero(&server_addr,sizeof(server_addr));
			server_addr.sin_family = AF_INET;
			if(inet_aton(addr.c_str(),&server_addr.sin_addr) == 0) 
			{
				printf("Server IP Address Error!\n");
				return 3;
			}
			server_addr.sin_port = htons(port);
			socklen_t server_addr_length = sizeof(server_addr);
			if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
			{
				printf("Can Not Connect !\n");
				return 4;
			}
			inited = true;
			return 0;
		}

};

class Benchmark {
	public:
		Benchmark() {}


		void bench() {
			struct timeval tvstart, tvend;
			int numbers = 100000;
			int du = 0;
			do_db_query query ;
			assert(0==query.init("127.0.0.1",3000));
			RandomGenerator gen;
			std::string key,ret;

			gettimeofday(&tvstart, NULL);	
			for (int i=0;i<numbers;i++)
			{
				gen.RandomString(&(gen.rand),16,&key);
				query.pingpong(key,ret);
				
				if (ret[0]=='1')
					du++;
					

				if (i%10000 == 0)
					printf("deal %d\n",i);
				//printf("==>%s\n",ret.c_str());
			}


			gettimeofday(&tvend, NULL);	
			int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
			printf("%d use %d(ms),du = %d\n",numbers,total,du);
			printf("%d transaction per second\n",numbers*1000/total);


		}


};

int main(int argc, char **argv)
{
	Benchmark b;
	b.bench();

}

/*
	 int main(int argc, char **argv)
	 {
	 if (argc != 2)
	 {
	 printf("Usage: ./%s ServerIPAddress\n",argv[0]);
	 exit(1);
	 }



	 struct sockaddr_in client_addr;
	 bzero(&client_addr,sizeof(client_addr)); 
	 client_addr.sin_family = AF_INET;   
	 client_addr.sin_addr.s_addr = htons(INADDR_ANY);
	 client_addr.sin_port = htons(0);   
	 int client_socket = socket(AF_INET,SOCK_STREAM,0);
	 if( client_socket < 0)
	 {
	 printf("Create Socket Failed!\n");
	 exit(1);
	 }
	 if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
	 {
	 printf("Client Bind Port Failed!\n"); 
	 exit(1);
	 }

	 struct sockaddr_in server_addr;
	 bzero(&server_addr,sizeof(server_addr));
	 server_addr.sin_family = AF_INET;
	 if(inet_aton(argv[1],&server_addr.sin_addr) == 0) 
	 {
	 printf("Server IP Address Error!\n");
	 exit(1);
	 }
	 server_addr.sin_port = htons(HELLO_WORLD_SERVER_PORT);
	 socklen_t server_addr_length = sizeof(server_addr);

	 if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
	 {
	 printf("Can Not Connect To %s!\n",argv[1]);
	 exit(1);
	 }

	 char buffer[BUFFER_SIZE];
	 char pbuf[BUFFER_SIZE];
	 bzero(buffer,BUFFER_SIZE);
	 int i=0;
	 sprintf(buffer,"%d",i);
	 int length = 0;

	 struct timeval tvstart, tvend;
	 gettimeofday(&tvstart, NULL);	
	 int numbers = 500000;

	 int du=0;
	 while (send(client_socket,buffer,strlen(buffer),0)>0) {

	 length = recv(client_socket,buffer,BUFFER_SIZE,0);
	 if(length < 0)
	 {
	 printf("Recieve Data From Server %s Failed!\n", argv[1]);
	 break;
	 } 
	 else 
	 {
	 if (buffer[0]=='1') 
	 du++;
//strncpy(pbuf,buffer,length);
//pbuf[length] ='\0';
//printf("DATA: %s\n",pbuf);
bzero(buffer,BUFFER_SIZE);    
i++;
sprintf(buffer,"%d",i);
}
if (i>=numbers)
	break;
	}

gettimeofday(&tvend, NULL);	
int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;

printf("%d use %d(ms),du = %d\n",numbers,total,du);
printf("%d transaction per second\n",numbers*1000/total);

//关闭socket

//closesocket(client_socket);
close(client_socket);
return 0;
}
*/
