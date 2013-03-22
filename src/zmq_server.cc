#include <leveldb/db.h>
#include "tools.h"
using namespace std;



int main(int argc, char *argv[])
{
	leveldb::DB *db;
	leveldb::Options options;
	options.create_if_missing = true;
	options.error_if_exists = false;
	leveldb::Status status = leveldb::DB::Open(options, "/tmp/fanglf_leveldb_ipc", &db);
	if(!status.ok()) {
		cerr<<status.ToString()<<endl;
		return 0;
	}

	void *ctx = zmq_init (1);
	assert (ctx);

	void *sb = zmq_socket (ctx, ZMQ_REP);
	assert (sb);
	int rc = zmq_bind (sb, "ipc:///tmp/fanglf_zero_ipc");
	//int rc = zmq_bind (sb, "tcp://127.0.0.1:3000");
	assert (rc == 0);
  
	string key="";
	string value="1";
	while (true) {
		my_msg_recv(sb,key) ;
		//my_msg_send(sb,"1");


		status = db->Get(leveldb::ReadOptions(), key, &value);
		if(!status.ok()) {
			db->Put(leveldb::WriteOptions(), key, value);
			value = "0";
		} else {
			value = "1";
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

