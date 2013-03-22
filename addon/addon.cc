#include <node.h>

#include <leveldb/db.h>
#include <map>
using namespace v8;

leveldb::DB *db = NULL;	
leveldb::Status status;



//假设是  单线程访问 
class db_pool { 
	private:
		std::map<std::string,leveldb::DB * >::iterator dbit;
		std::map<std::string,leveldb::DB * > _db_pool;
		std::string _db_path;
	public:
		void init() {
				leveldb::Options options;	
				options.create_if_missing = true;	
				options.error_if_exists = false;	
				status = leveldb::DB::Open(options, "config", &db);
				if (!status.ok()) {
				  // default config;
					_db_path = "/tmp/";
				} else {
					// leveldb where be create in db_prefix 
					status = db->Get(leveldb::ReadOptions(), "db_prefix", &_db_path);		
					if (!status.ok())
						_db_path = "/tmp/";
				}
				delete db;
				db = NULL;
		}

		leveldb::DB* getDbByName(std::string dbname) {
			dbit=_db_pool.find(dbname);
			if (dbit == _db_pool.end()) {
				leveldb::Options options;	
				options.create_if_missing = true;	
				options.error_if_exists = false;	
				status = leveldb::DB::Open(options, _db_path+dbname, &db);
				_db_pool[dbname]=db;
				return db;
			} else {
				return dbit->second;
			}
		}
		
		void close(std::string dbname) {
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
		status = db->Get(leveldb::ReadOptions(), *key, &value);		
		if(!status.ok()) {			
			db->Put(leveldb::WriteOptions(), *key, *invalue);		
			Local<Number> num = Number::New(0);
			return scope.Close(num);
		} else {
			Local<Number> num = Number::New(1);
			return scope.Close(num);
		}
}

/*
Handle<Value> Close(const Arguments& args) {
	  HandleScope scope;
		if (args.Length() < 1) {
			ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
			return scope.Close(Undefined());
		}

		v8::String::AsciiValue pdbname(args[0]); 
		_pool.close(*pdbname);
		
		return scope.Close(v8::Null());
}
*/

Handle<Value> Query(const Arguments& args) {
	  HandleScope scope;
		if (args.Length() < 2) {
			ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
			return scope.Close(Undefined());
		}

		static Persistent<String> ok;
		static Persistent<String> v;
    ok = NODE_PSYMBOL("ok");
		v = NODE_PSYMBOL("v");
		Local<Object> info = Object::New();

		std::string value ;
		v8::String::AsciiValue key(args[0]); 
		v8::String::AsciiValue dbname(args[1]); 
		db = _pool.getDbByName(*dbname);
		status = db->Get(leveldb::ReadOptions(), *key, &value);		
		if(!status.ok()) {			
			info->Set(ok, String::New("1"));
			info->Set(v, String::Empty());
			return scope.Close(info);
		} else {
			info->Set(ok, String::New("0"));
			info->Set(v, String::New(value.c_str()));
			return scope.Close(info);
		}
}

/*
Handle<Value> Open(const Arguments& args) {
	  HandleScope scope;
		if (args.Length() < 1) {
			ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
			return scope.Close(Undefined());
		}

		v8::String::AsciiValue path(args[0]); 
		leveldb::Options options;	
		options.create_if_missing = true;	
		options.error_if_exists = false;	

		status = leveldb::DB::Open(options, *path, &db);
		if(!status.ok()) {			
			ThrowException(Exception::TypeError(String::New(status.ToString().c_str())));
			return scope.Close(Undefined());
		}

		return scope.Close(v8::Null());
}
*/

void Init(Handle<Object> target) {
	  target->Set(String::NewSymbol("add"),
				      FunctionTemplate::New(Add)->GetFunction());
		target->Set(String::NewSymbol("query"),
				      FunctionTemplate::New(Query)->GetFunction());
		_pool.init();
}

NODE_MODULE(addon, Init)
