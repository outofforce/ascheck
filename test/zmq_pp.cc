#include <leveldb/db.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <vector>
#include <signal.h>
#include <assert.h>
#include <map>

#include "tools.h"
using namespace std;


struct task{
	int fd;            
	struct task *next; 
	std::string head ;
	std::string data ;
};

/*
struct user_data{
	int fd;
	unsigned int n_size;
	char line[MAXLINE];
};
*/

void * readtask(void *args);
//void * writetask(void *args);
pthread_mutex_t mutex;
pthread_cond_t cond1;
struct task *readhead=NULL,*readtail=NULL,*writehead=NULL;


void ctrl_c(int sig)
{
	printf("CTRL+C, ...\n");
	//b_ctrl_c = true;
	exit(0);
}


void *ctx;
class channel_pool { 
	private:
		std::map<std::string,void * >::iterator dbit;
		std::map<std::string,void * > _channel_pool;
		std::string _channel_uri;
		int rc ;
	public:
		void init() {
			rc = 0;
		  _channel_uri = "ipc:///tmp/fanglf_to_server";

		}

		void* getChannelByName(std::string chname) {
			dbit=_channel_pool.find(chname);
			if (dbit == _channel_pool.end()) {

				void *sb = zmq_socket (ctx, ZMQ_PUSH);
				assert (sb);

				int rc = zmq_connect(sb, (_channel_uri+"_"+chname).c_str());
				assert (rc == 0);
				_channel_pool[chname]=sb;
				return sb;
			} else {
				return dbit->second;
			}
		}
		
		void close(std::string chname) {
			dbit=_channel_pool.find(chname);
			if (dbit != _channel_pool.end()) {

				if (dbit->second != NULL) {
					rc = zmq_close (dbit->second);
					assert (rc == 0);
					dbit->second = NULL;
				}
				_channel_pool.erase(dbit);

			}
			if (ctx != NULL) {
				rc = zmq_term (ctx);
				assert (rc == 0);
			}
		}

		~channel_pool() {

			for (dbit=_channel_pool.begin();dbit!=_channel_pool.end();dbit++)
			{
				if (dbit->second != NULL)  {
					rc = zmq_close (dbit->second);
					assert (rc == 0);
					dbit->second = NULL;
				}
			}

			if (ctx != NULL) {
				rc = zmq_term (ctx);
				assert (rc == 0);
			}


		}
};

channel_pool pool;



int main(int argc, char *argv[])
{
	signal(SIGINT, ctrl_c);


	pthread_t tid1,tid2;
	struct task *new_task = NULL;
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond1, NULL);

	
	ctx = zmq_ctx_new();
	assert (ctx);
	pool.init();
	pthread_create(&tid1, NULL, readtask, NULL);
	void *sb = zmq_socket (ctx, ZMQ_PULL);
	assert (sb);
	int rc = zmq_bind (sb, "ipc:///tmp/fanglf_zero_ipc");
	//int rc = zmq_bind (sb, "tcp://127.0.0.1:3000");
	assert (rc == 0);
  
	string key="";
	string value="1";
	int i=0;

	
	
	string head;
	while (true) {
		my_msg_recv(sb,key) ;
		head=key.substr(0,8);
		value = key.substr(8,key.length()-8);
		

		new_task = new task();
		new_task->fd = 0;
		new_task->next = NULL;
		new_task->head = head;
		new_task->data = value;

		// create a read task 
		pthread_mutex_lock(&mutex);

		if(readhead == NULL)
		{
			readhead = new_task;
			readtail = new_task;
		}
		else
		{
			readtail->next = new_task;
			readtail = new_task;
		}
		pthread_cond_broadcast(&cond1);
		pthread_mutex_unlock(&mutex);

	}

	rc = zmq_close (sb);
	assert (rc == 0);


	rc = zmq_term (ctx);
	assert (rc == 0);

	return 0;

}
void * readtask(void *args)
{
	int fd=-1;
	int n;
	int type;
	long read_len=0; 
	char read_buffer[1024]; 
	std::string data;
	std::string head;
	std::string tvalue;
	int dealcount=0;

	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = false;
	leveldb::Status status = leveldb::DB::Open(options, "/tmp/fanglf_leveldb_ipc", &db);
	if(!status.ok()) {
		cerr<<status.ToString()<<endl;
		return 0;
	}

	void *to_server;// = zmq_socket (ctx, ZMQ_PUSH);
	//assert (to_server);
	//int rc = zmq_connect (to_server, "ipc:///tmp/fanglf_to_server");
	//int rc = zmq_connect(sb, "tcp://127.0.0.1:3000");
	//assert (rc == 0);
  
	printf("readtask working\n");
	while(1){

		pthread_mutex_lock(&mutex);

		while(readhead == NULL)
			pthread_cond_wait(&cond1, &mutex); 

		fd = readhead->fd;
		data = readhead->data;
		head = readhead->head;

		struct task *tmp = readhead;
		readhead = readhead->next;
		delete tmp;
		pthread_mutex_unlock(&mutex);

		to_server = pool.getChannelByName(head); 

		status = db->Get(leveldb::ReadOptions(), data, &tvalue);
		if(!status.ok()) {
			db->Put(leveldb::WriteOptions(), data, tvalue);
			tvalue = "0";
		} else {
			tvalue = "1";
		}
		dealcount++;

		if (dealcount%10000 == 0)
			printf("del %d\n",dealcount);

		my_msg_send(to_server,head+tvalue);
	}

	delete db;
}

