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
#include <map>
#include <zmq.h>
#include "ascheck.h"

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
*/

#define BUFFER_SIZE 128 

struct svc_item {
	std::string addr;
	int port;
	bool connected;
	int socket_fd;
	int count;
	svc_item():count(0),socket_fd(0),connected(false),port(0) {}
};

class service_domain {
	std::string _server_addr;
	int _s_count;
	std::map<int,svc_item> _serv_map;
	std::map<int,svc_item>::iterator _servit;
	bool _inited;
	char buffer[BUFFER_SIZE];
	
	int _getServerAddr() {
		// TODO get this from shell context
		_server_addr="tcp://127.0.0.1:3000";
		return 0;
	}

	void my_msg_send(void *sc, const std::string &data) {

		if (data.size() == 0) return ;

		static char send_buf[BUFFER_SIZE];
		int pos=0;
		int rc=0;

		while (pos+BUFFER_SIZE< data.length()) {

			rc = zmq_send (sc, data.data()+pos, BUFFER_SIZE, ZMQ_SNDMORE);
			assert(rc == BUFFER_SIZE);
			pos += BUFFER_SIZE;
		}

		rc = zmq_send (sc, data.data()+pos, data.length()-pos, 0);

	}


	void my_msg_recv(void *sb, std::string &data) {

		static char rev_buf[BUFFER_SIZE];

		int rcvmore=true;
		size_t sz = sizeof (rcvmore);
		int rc=0;

		data="";
		while (rcvmore) {
			bzero(rev_buf,BUFFER_SIZE);
			rc = zmq_recv(sb, rev_buf, BUFFER_SIZE, 0);
			assert (rc != -1);
			// no '\0' while be recieved
			data = data + std::string(rev_buf,rc);
			rc = zmq_getsockopt (sb, ZMQ_RCVMORE, &rcvmore, &sz);
		}
	}


	int init_socket(const std::string &addr,int port) {

		struct sockaddr_in client_addr;
		bzero(&client_addr,sizeof(client_addr)); 
		client_addr.sin_family = AF_INET;   
		client_addr.sin_addr.s_addr = htons(INADDR_ANY);
		client_addr.sin_port = htons(0);   

		int client_socket;
		client_socket = socket(AF_INET,SOCK_STREAM,0);
		if( client_socket < 0)
		{
			printf("Create Socket Failed!\n");
			return -1;
		}
		if( bind(client_socket,(struct sockaddr*)&client_addr,sizeof(client_addr)))
		{
			printf("Client Bind Port Failed!\n"); 
			return -2;
		}

		struct sockaddr_in server_addr;
		bzero(&server_addr,sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		if(inet_aton(addr.c_str(),&server_addr.sin_addr) == 0) 
		{
			printf("Server IP Address Error!\n");
			return -3;
		}
		server_addr.sin_port = htons(port);
		socklen_t server_addr_length = sizeof(server_addr);
		if(connect(client_socket,(struct sockaddr*)&server_addr, server_addr_length) < 0)
		{
			printf("Can Not Connect !\n");
			return -4;
		}
		return client_socket;
	}

	public:
	service_domain():_inited(false){}
	int init() {
		if (_inited) return 0;
		int rc = _getServerAddr();
		if (rc !=0) return rc;

		void *ctx = zmq_ctx_new();
		assert (ctx);

		void *pb = zmq_socket (ctx, ZMQ_REQ);
		assert (pb);
		rc = zmq_connect(pb, _server_addr.c_str());
		assert (rc == 0);

		std::string key,buf;

		key = "SERVICE_COUNT";
		my_msg_send(pb,key);
		my_msg_recv(pb,buf);
		assert(buf.length()>0);
		char num[128];
		_s_count = atoi(buf.c_str());
		printf("total service num = %d\n",_s_count);

		// TODO need check error !
		for (int j=0;j<_s_count;j++) {
			int i=j+1; 
			svc_item item;
			sprintf(num,"ASCHECK_ADDR_%d",i);	
			my_msg_send(pb,num);
			my_msg_recv(pb,buf);
			assert(buf.length()>0);
			item.addr=buf;
			sprintf(num,"ASCHECK_PORT_%d",i);	
			my_msg_send(pb,num);
			my_msg_recv(pb,buf);
			assert(buf.length()>0);
			item.port=atoi(buf.c_str());
			item.connected = false;
			item.socket_fd = 0;
			printf("service_index(%d) %s:%d\n",i,item.addr.c_str(),item.port);
			_serv_map[i]=item;
		}

		rc = zmq_close (pb);
		assert (rc == 0);
		rc = zmq_term (ctx);
		assert (rc == 0);
		_inited = true;
		return 0;

	}

	int pingpong(int index,const std::string &sbuf,std::string &rbuf) {

		int client_socket = getSvc(index);
		int ret = send(client_socket,sbuf.c_str(),sbuf.length(),0);
		if (ret < 0) {
			printf("Send Data Error!");
			return -6;
		}

		bzero(buffer,BUFFER_SIZE);
		int length = recv(client_socket,buffer,BUFFER_SIZE,0);
		if(length < 0)
		{
			printf("Recieve Data Error!");
			return -7;
		} 
		else 
		{
			buffer[length]='\0';
			rbuf = buffer;
		}

		return 0;

	}



	int getSvcCount() {return _s_count;}

	// return a socket fd
	int getSvc(int index) {
			
		_servit =_serv_map.find(index);
		if (_servit== _serv_map.end()) {
			assert(false);
			return 0;
		} else {

			if (_servit->second.connected)
			{
				_servit->second.count++;
				return _servit->second.socket_fd;
				}
			else {
				int rc = init_socket(_servit->second.addr,_servit->second.port);
				assert(rc > 0);
				_servit->second.count++;
				_servit->second.socket_fd = rc;
				_servit->second.connected = true;
				return rc;
			}
		}
	}

	void closeByIndex(int index) {
		_servit =_serv_map.find(index);
		if (_servit!=_serv_map.end()) {
			if (_servit->second.connected) {
				close(_servit->second.socket_fd);
			}
			_servit->second.connected = false;
		}
	}

	void logCount() {
		for (_servit = _serv_map.begin();
				_servit != _serv_map.end();
				_servit++)
		{

			if (_servit->second.connected == true) {
				printf("  serv(%s:%d) handle request %d\n",
						_servit->second.addr.c_str(),
						_servit->second.port,
						_servit->second.count);
			}
		}
	}

	~service_domain() {

		logCount();
		for (_servit = _serv_map.begin();
				 _servit != _serv_map.end();
				 _servit++)
		{
			
			if (_servit->second.connected == true) {
				//printf("close %d\n",_servit->second.socket_fd);
				close(_servit->second.socket_fd);
				//printf("close NULL\n");
				_servit->second.connected = false;
			}
		}
	}

};

class ashash {
	static unsigned long DJBhash(const char *p ,int len) {
			unsigned long hash = 5381;
			for (int i=0;i<len;i++)
				hash = ((hash<<5)+hash)+p[i];
			return hash;
	}
	public:
		static int getHashIndex(const std::string &key,const int count) {
			//printf("%ld\n",DJBhash(key.c_str(),key.length())%count);
			return (int)(DJBhash(key.c_str(),key.length())%count + 1);

		}
};

// 1 add 
// 2 query
// 3 del
class asproto {
	public:
	static std::string makeAddProto(
			const std::string &part,
			const std::string &key,
			const std::string &value
			) 
	{
		char buf[BUFFER_SIZE];
		assert(10+part.length()+key.length()+value.length() < BUFFER_SIZE);
		sprintf(buf,"%02d%02d%04d%02d%s%s%s",1,
				part.length(),key.length(),value.length(),
				part.c_str(),key.c_str(),value.c_str());
		
		//printf("%s   %s   %s\n",part.c_str(),key.c_str(),value.c_str());
		//printf("%s\n",buf);
		return buf;

	}

	static std::string makeDelProto(
			const std::string &part,
			const std::string &key
			) 
	{
		char buf[BUFFER_SIZE];
		assert(10+part.length()+key.length() < BUFFER_SIZE);
		sprintf(buf,"%02d%02d%04d%02d%s%s%s",3,
				part.length(),key.length(),0,
				part.c_str(),key.c_str(),"");
		
		//printf("%s   %s  \n",part.c_str(),key.c_str());
		//printf("%s\n",buf);
		return buf;
	}

	static std::string makeQueryProto(
			const std::string &part,
			const std::string &key
			) 
	{
		char buf[BUFFER_SIZE];
		assert(10+part.length()+key.length() < BUFFER_SIZE);
		sprintf(buf,"%02d%02d%04d%02d%s%s%s",2,
				part.length(),key.length(),0,
				part.c_str(),key.c_str(),"");
		
		//printf("%s   %s  \n",part.c_str(),key.c_str());
		//printf("%s\n",buf);
		return buf;
	}

	// data = 00somedata
	// 00 success 
	//  
	static int pareProto(
			const std::string &data,
			std::string &value
			) 
	{
		assert(data.length()>1);
		std::string t = data.substr(0,2);
		value = data.substr(2,data.length()-2);
		return atoi((data.substr(0,2)).c_str());
	}

};


/*
 *  if key is in db , return 1
 *  else inset key/value to db , and return 0;
 *  err happen ,return <0 ,
 */ 

service_domain domain;

int ascheck::add(
	const std::string &part,
	const std::string &key,
	const std::string &value) {

	int rc = domain.init() ;
	assert(rc ==0);

	int index = ashash::getHashIndex(key,domain.getSvcCount());
	//printf("index = -1? %d\n",index);
	static std::string v1,v2;
	v1 = asproto::makeAddProto(part,key,value);
	rc = domain.pingpong(index,v1,v2);
	assert(rc==0);
	rc = asproto::pareProto(v2,v1);

	if (rc==0)
		return 0;
	else if (rc == 1)
		return 1;
	else
		return -1;
}

/*
 *  success return 0;
 *  no this value return 1;
 *  err happen ,return <0 ,
 */ 

int ascheck::query(
	const std::string &part,
	const std::string &key,
	std::string &value) {

	int rc = domain.init() ;
	assert(rc ==0);

	static std::string v1,v2;
	int index = ashash::getHashIndex(key,domain.getSvcCount());
	v1 = asproto::makeQueryProto(part,key);
	rc = domain.pingpong(index,v1,v2);
	assert(rc==0);
	rc = asproto::pareProto(v2,v1);

	if (rc==0) {
	  value = v1;
		return 0;
	}
	else if (rc == 1) {
		value = "";
		return 1;
	}
	else
		return -1;

	return 0;
}

/*
 *  success return 0;
 *  err happen ,return <0 ,
 */ 
int ascheck::del(
	const std::string &part,
	const std::string &key) {

	int rc = domain.init() ;
	assert(rc ==0);

	static std::string v1,v2;
	int index = ashash::getHashIndex(key,domain.getSvcCount());
	v1 = asproto::makeDelProto(part,key);
	rc = domain.pingpong(index,v1,v2);
	assert(rc==0);
	rc = asproto::pareProto(v2,v1);

	if (rc==0)
		return 0;
	else
		return -1;

	return 0;
}
