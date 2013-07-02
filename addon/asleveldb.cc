#include <node.h> 
#include <leveldb/db.h>
#include <openssl/hmac.h>
#include <sys/time.h>


#include <map>
#include <list>
using namespace v8;


class asLRU_cache {
	private:
		size_t _capacity;
		std::list<std::string> _cache;
		std::list<std::string>::iterator _it;
		//std::map<std::string,int> _index;
		//std::map<std::string,int>::iterator _index_it;

	public:
		asLRU_cache(size_t capacity):_capacity(capacity) {
		}
		void setCapacity(size_t capacity) { _capacity = capacity ;} 
		int pullLeastUse(std::string &name) {
			if (_cache.empty()) return -1;
			_it = _cache.end();
			_it--;
			name = (*_it);
			_cache.erase(_it);
			return 0;
		}

		int use(const std::string &index) {
			_it = find(index);
			if (_it != _cache.end())
				_cache.erase(_it);
			
			_cache.push_front(index);
			return 0;
		}
		bool isFull() { return _cache.size() > _capacity; }

		void log() {
			for (_it=_cache.begin();_it != _cache.end() ;_it++)
				printf("%s\t",(*_it).c_str());
			printf("\n");
		}

		std::list<std::string>::iterator  find(const std::string &name) {
			for (_it=_cache.begin();_it != _cache.end() ;_it++)
				if ((*_it) == name) return _it;
			return _cache.end();
		}
};


class asRandom {
	private:
		uint32_t seed_;
	public:

		asRandom() : seed_(0) { }
		void setSeed(uint32_t s) { seed_= s & 0x7fffffffu;  }


		uint32_t Next() {
			static const uint32_t M = 2147483647L;   // 2^31-1
			static const uint64_t A = 16807;  
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

leveldb::DB *db = NULL;	
leveldb::Status status;
asRandom asrnd;
asLRU_cache asLRU(20);


void asRandomString(int len, std::string* dst) {
	dst->resize(len);
	for (int i = 0; i < len; i++) {
		(*dst)[i] = static_cast<char>(' ' + asrnd.Uniform(95));  
	}
}	


int ashmac(const std::string &data, std::string &digest) {

	HMAC_CTX ctx;
	const char *key="dfs3f00_)*(&98634sdfasd;ldj0830iuoremzi0oer'zddf0s782-0234fjdfat";
	unsigned char mdvalue[EVP_MAX_MD_SIZE];
	unsigned int vlen;
	const EVP_MD *md = EVP_sha256();
	if(!md)
		return -1;

	HMAC_CTX_init(&ctx);
	HMAC_Init(&ctx,key,strlen(key), md);
	HMAC_Update(&ctx, (unsigned char*)data.c_str(), strlen(data.c_str()));
	HMAC_Final(&ctx, mdvalue,&vlen);
	HMAC_CTX_cleanup(&ctx);
	unsigned char out[EVP_MAX_MD_SIZE*2+1];
	char *p = (char *)out;
	for (int i=0;i<(int)vlen;i++)
	{
		sprintf(p,"%02x",(unsigned int)mdvalue[i]);
		p += 2;
	}
	*p='\0';
	digest = (char*)out;
	return 0;
}

std::string asGetUUID() {
	std::string out;
	asRandomString(128,&out);
	return out;
}

int getDataFromString(
		const std::string &src,
		const std::string &key,
		std::string &value) {
	size_t pos = src.find(key+"=");
	if (pos == std::string::npos)
		return -1;
	else {
		pos = pos+key.size()+1;
		size_t e_pos = src.find(";",pos);
		if (e_pos == std::string::npos) e_pos = src.size(); 
		if (e_pos-pos>0) {
			value = src.substr(pos,e_pos-pos);
			if (value == "NULL")
				return -1;
			else 
				return 0;
		} else {
			return -1;
		}
	}
}

int getDataFromString(
		const std::string &src,
		const std::string &key,
		size_t &value) {
	size_t pos = src.find(key+"=");
	if (pos == std::string::npos)
		return -1;
	else {
		pos = pos+key.size()+1;
		size_t e_pos = src.find(";",pos);
		if (e_pos == std::string::npos) e_pos = src.size(); 
		if (e_pos-pos>0) {
			std::string t= src.substr(pos,e_pos-pos);
			if (t== "NULL")
				return -1;
			else  {
				value = (size_t)atol(t.c_str());
				return 0;
			}
		} else {
			return -1;
		}
	}
}


class db_pool { 
	private:
		std::map<std::string,leveldb::DB * >::iterator dbit;
		std::map<std::string,leveldb::DB * > _db_pool;
		std::string _db_path;
		leveldb::CompressionType _db_comp_type;
		int _db_max_open_files;
		size_t _db_write_buffer_size;
		size_t _db_block_size;
		int _accessCounter;

		void _loadDbConfig(const std::string &src) {


			_db_comp_type = leveldb::kNoCompression;
			_db_max_open_files = 0; // use level default
			_db_write_buffer_size =0; // use level default
			_db_block_size =0; // use level default 
			size_t t_v;
			if (getDataFromString(src,"DB_CompressionType",t_v) == 0) {
				if (t_v == 1) {
					_db_comp_type=leveldb::kSnappyCompression;
					printf("\tDB_CONFIG: use snappy compression\n");
				}
			} 

			if (getDataFromString(src,"DB_block_size",t_v) == 0) {
				_db_block_size=t_v;
				printf("\tDB_CONFIG: block_size = %lu\n",t_v);
			} 

			if (getDataFromString(src,"DB_write_buffer_size",t_v) == 0) {
				_db_write_buffer_size=t_v;
				printf("\tDB_CONFIG: write_buffer_size= %lu\n",t_v);
			} 

			if (getDataFromString(src,"DB_max_open_files",t_v) == 0) {
				_db_max_open_files=(int)t_v;
				printf("\tDB_CONFIG: max_open_files = %lu\n",t_v);
			} 
		}

	public:
		void init(const std::string &index,const std::string &prefix,const std::string &dbarg) {
			if (index.length() == 0)
				_db_path = prefix+"/";
			else 
				_db_path = prefix+"/"+index+"_";

			_loadDbConfig(dbarg);

			_accessCounter=0;
			//printf("%s\n",_db_path.c_str());

		}

		leveldb::DB* getDbByName(const std::string &dbname) {
			asLRU.use(dbname);
			_accessCounter++;
			if (_accessCounter > 10000) {
				std::string closedbname;
				while (asLRU.isFull()) {
					if (0==asLRU.pullLeastUse(closedbname))
						closeByName(closedbname);
				}
				_accessCounter=0;
			}


			dbit=_db_pool.find(dbname);
			if (dbit == _db_pool.end()) {
				leveldb::Options options;	
				options.create_if_missing = true;	
				options.error_if_exists = false;	
				if (_db_max_open_files>0)
					options.max_open_files = _db_max_open_files;

				if (_db_block_size>0) options.block_size= _db_block_size;

				if (_db_write_buffer_size>0)
					options.write_buffer_size= _db_write_buffer_size;

				options.compression= _db_comp_type;;

				printf("open db ctx = %s\n",(_db_path+dbname).c_str());
				status = leveldb::DB::Open(options, _db_path+dbname, &db);

				if (!status.ok()) {
					ThrowException(
							Exception::TypeError(String::New(status.ToString().c_str())));
				}

				_db_pool[dbname]=db;
				return db;
			} else {
				return dbit->second;
			}
		}

		void closeByName(std::string dbname) {
			dbit=_db_pool.find(dbname);
			if (dbit != _db_pool.end()) {
				if (dbit->second != NULL) {
					delete dbit->second;
				}
				_db_pool.erase(dbit);
			}
		}

		~db_pool() {
			for (dbit=_db_pool.begin();dbit!=_db_pool.end();dbit++)
				if (dbit->second != NULL) 
					delete dbit->second;
		}
};

db_pool _pool;


Handle<Value> Add(const Arguments& args) {
	HandleScope scope;

	if (args.Length() < 3) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	std::string value;
	v8::String::AsciiValue key(args[0]); 
	v8::String::AsciiValue invalue(args[1]); 
	v8::String::AsciiValue dbname(args[2]); 
	db = _pool.getDbByName(*dbname);

	Local<Number> num ;
	status = db->Get(leveldb::ReadOptions(), *key, &value);		
	if(!status.ok()) {			
		//printf("err %s\n",status.ToString().c_str());
		db->Put(leveldb::WriteOptions(), *key, *invalue);		
		num = Number::New(0);
	} else {
		num = Number::New(1);
	}
	return scope.Close(num);
}

Handle<Value> Write(const Arguments& args) {
	HandleScope scope;

	if (args.Length() < 3) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	std::string value;
	v8::String::AsciiValue key(args[0]); 
	v8::String::AsciiValue invalue(args[1]); 
	v8::String::AsciiValue dbname(args[2]); 
	db = _pool.getDbByName(*dbname);
	status = db->Put(leveldb::WriteOptions(), *key, *invalue);		
	if(!status.ok()) {			
		//printf("err %s\n",status.ToString().c_str());
		Local<Number> num = Number::New(1);
		return scope.Close(num);
	} else {
		Local<Number> num = Number::New(0);
		return scope.Close(num);
	}
}

Handle<Value> Del(const Arguments& args) {
	HandleScope scope;

	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	std::string value;
	v8::String::AsciiValue key(args[0]); 
	v8::String::AsciiValue dbname(args[1]); 
	db = _pool.getDbByName(*dbname);
	status = db->Delete(leveldb::WriteOptions(), *key);		
	Local<Number> num = Number::New(1);
	if(!status.ok()) {			
		//printf("err %s\n",status.ToString().c_str());
		num = Number::New(1);
	} else {
		num = Number::New(0);
	}
	return scope.Close(num);
}

Handle<Value> QueryCfg(const Arguments& args) {
	HandleScope scope;
	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	Local<Object> info = Object::New();

	std::string value = "";
	v8::String::AsciiValue key(args[0]); 
	v8::String::AsciiValue dbname(args[1]); 
	db = _pool.getDbByName(*dbname);
	status = db->Get(leveldb::ReadOptions(), *key, &value);		
	if(!status.ok()) {			
		info->Set(String::New("ok"), Number::New(1));
		info->Set(String::New("v"), String::Empty());
	} else {
		info->Set(String::New("ok"), Number::New(0));
		info->Set(String::New("v"), String::New(value.c_str()));
	}
	return scope.Close(info);
}

Handle<Value> Query(const Arguments& args) {
	HandleScope scope;
	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	Local<Object> info = Object::New();

	std::string value = "";
	v8::String::AsciiValue key(args[0]); 
	v8::String::AsciiValue dbname(args[1]); 
	db = _pool.getDbByName(*dbname);
	status = db->Get(leveldb::ReadOptions(), *key, &value);		
	if(!status.ok()) {			
		//printf("err %s\n",status.ToString().c_str());
		info->Set(String::New("ok"), Number::New(1));
		info->Set(String::New("v"), String::Empty());
	} else {
		info->Set(String::New("ok"), Number::New(0));
		info->Set(String::New("v"), String::New(value.c_str()));
	}
	return scope.Close(info);
}


Handle<Value> getRandom(const Arguments& args) {
	HandleScope scope;
	std::string value = asGetUUID();
	return scope.Close(String::New(value.c_str()));
}

Handle<Value> asAuth(const Arguments& args) {
	HandleScope scope;

	if (args.Length() < 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	v8::String::AsciiValue randomdata(args[0]); 
	v8::String::AsciiValue data(args[1]); 

	//printf("err %d,%s,%s\n",args.Length(),*randomdata,*data);

	std::string asdigest;
	int rc = ashmac(*randomdata,asdigest); 
	//printf("err==: %s\n",asdigest.c_str());
	if (rc !=0) {
		ThrowException(Exception::TypeError(String::New("Unkown message digest sha256")));
		return scope.Close(Undefined());
	}

	Local<Number> num ;
	//printf("err %s\n",asdigest.c_str());
	if(asdigest == std::string(*data)) {			
		num = Number::New(0);
	} else {
		num = Number::New(1);
	}
	return scope.Close(num);
}



Handle<Value> DbInit(const Arguments& args) {
	HandleScope scope;
	if (args.Length() < 1) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	String::AsciiValue path(args[0]);
	String::AsciiValue index(args[1]);
	String::AsciiValue dbconfig(args[2]);

	_pool.init(*index,*path,*dbconfig);
	Local<Number> num = Number::New(0);
	return scope.Close(num);
}


void Init(Handle<Object> target) {
	target->Set(String::NewSymbol("add"),
			FunctionTemplate::New(Add)->GetFunction());
	target->Set(String::NewSymbol("query"),
			FunctionTemplate::New(Query)->GetFunction());
	target->Set(String::NewSymbol("del"),
			FunctionTemplate::New(Del)->GetFunction());
	target->Set(String::NewSymbol("write"),
			FunctionTemplate::New(Write)->GetFunction());

	target->Set(String::NewSymbol("getRandom"),
			FunctionTemplate::New(getRandom)->GetFunction());

	target->Set(String::NewSymbol("asAuth"),
			FunctionTemplate::New(asAuth)->GetFunction());

	target->Set(String::NewSymbol("queryCfg"),
			FunctionTemplate::New(QueryCfg)->GetFunction());




	target->Set(String::NewSymbol("dbinit"),
			FunctionTemplate::New(DbInit)->GetFunction());

	struct timeval tvs;
	gettimeofday(&tvs, NULL);
	asrnd.setSeed((uint32_t)(tvs.tv_usec%1000000));



}

NODE_MODULE(asleveldb, Init)
