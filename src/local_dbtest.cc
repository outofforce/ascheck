#include <leveldb/db.h>
#include "tools.h"
#include <sys/time.h>
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

  
	struct timeval tvstart, tvend;
	int numbers = 100000;
	int du = 0;
	RandomGenerator gen;
	std::string key,ret,value="0";

	gettimeofday(&tvstart, NULL);	
	for (int i=0;i<numbers;i++)
	{
		if (i%10000 == 0)
			printf("deal %d\n",i);
		gen.RandomString(&(gen.rand),16,&key);

		status = db->Get(leveldb::ReadOptions(), key, &value);
		if(!status.ok()) {
			db->Put(leveldb::WriteOptions(), key, value);
			value = "0";
		} else {
			value = "1";
			du++;
		}

	}

	gettimeofday(&tvend, NULL);	
	int total = (tvend.tv_sec-tvstart.tv_sec)*1000+(tvend.tv_usec-tvstart.tv_usec)/1000;
	printf("%d use %d(ms),du = %d\n",numbers,total,du);
	printf("%d transaction per second\n",numbers*1000/total);


	delete db;
	return 0;

}

