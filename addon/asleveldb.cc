#include <node.h>

#include <leveldb/db.h>
#include <map>
using namespace v8;

leveldb::DB *db = NULL;	
leveldb::Status status;



class db_pool { 
	private:
		std::map<std::string,leveldb::DB * >::iterator dbit;
		std::map<std::string,leveldb::DB * > _db_pool;
		std::string _db_path;
	public:
		void init(const std::string &index,const std::string &prefix) {
			if (index.length() == 0)
					_db_path = prefix+"/";
			else 
				 _db_path = prefix+"/"+index+"_";


			//printf("%s\n",_db_path.c_str());

		}

		leveldb::DB* getDbByName(std::string dbname) {
			dbit=_db_pool.find(dbname);
			if (dbit == _db_pool.end()) {
				leveldb::Options options;	
				options.create_if_missing = true;	
				options.error_if_exists = false;	
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


Handle<Value> DbInit(const Arguments& args) {
	  HandleScope scope;
		if (args.Length() < 1) {
			ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
			return scope.Close(Undefined());
		}

		String::AsciiValue path(args[0]);
		String::AsciiValue index(args[1]);

		_pool.init(*index,*path);
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

	target->Set(String::NewSymbol("dbinit"),
			FunctionTemplate::New(DbInit)->GetFunction());


}

NODE_MODULE(asleveldb, Init)
