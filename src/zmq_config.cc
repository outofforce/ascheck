#include <leveldb/db.h>
#include "tools.h"
using namespace std;



int main(int argc, char *argv[])
{
	std::string config_db_path = "/tmp/config";
	std::string uri = "tcp://127.0.0.1:3000";
	int c,count = 1;


	while ( (c = getopt(argc, argv, "u:d:")) != -1) {
		switch (c) {
			case 'u':
				uri = optarg;
				break;
			case 'd':
				config_db_path = optarg;
				break;
		}
	}


	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = false;
	leveldb::Status status = leveldb::DB::Open(
																	options, 
																	config_db_path.c_str(), 
																	&db);

	if(!status.ok()) {
		cerr<<status.ToString()<<endl;
		return 0;
	}

	void *ctx = zmq_init (1);
	assert (ctx);

	void *sb = zmq_socket (ctx, ZMQ_REP);
	assert (sb);
	int rc = zmq_bind (sb, uri.c_str());
	assert (rc == 0);
  
	string key="";
	string value;
	while (true) {
		my_msg_recv(sb,key) ;
		//printf("rvc key = %s\n",key.c_str());
		status = db->Get(leveldb::ReadOptions(), key, &value);
		if(!status.ok()) {
			value = "NULL";
		} 
		my_msg_send(sb,value);
	}

	rc = zmq_close (sb);
	assert (rc == 0);
	rc = zmq_term (ctx);
	assert (rc == 0);

	delete db;
	return 0;
}

