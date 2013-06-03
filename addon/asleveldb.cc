#include <node.h>

#include <leveldb/db.h>
#include <map>
using namespace v8;

leveldb::DB *db = NULL;	
leveldb::Status status;

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
			
			//printf("%s\n",_db_path.c_str());

		}

		leveldb::DB* getDbByName(std::string dbname) {
			dbit=_db_pool.find(dbname);
			if (dbit == _db_pool.end()) {
				leveldb::Options options;	
				options.create_if_missing = true;	
				options.error_if_exists = false;	
				if (_db_max_open_files>0)
					options.max_open_files = _db_max_open_files;

				if (_db_block_size>0)
					options.block_size= _db_block_size;

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

	target->Set(String::NewSymbol("dbinit"),
			FunctionTemplate::New(DbInit)->GetFunction());


}

NODE_MODULE(asleveldb, Init)
