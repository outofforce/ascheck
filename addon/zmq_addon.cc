#include <zmq.h>
#include <string>
#include <string.h>        // for bzero
#include <iostream>
#include <assert.h>
#include <stdint.h>    // for socket
#include <node.h>   
#include <map>   


#define REV_BUF_SIZE 128 

using namespace v8;



inline void my_msg_send(void *sc, const std::string &data) {

	if (data.size() == 0) return ;

	unsigned int pos=0;
	int rc=0;

	while (pos+REV_BUF_SIZE < data.length()) {
		
    rc = zmq_send (sc, data.data()+pos, REV_BUF_SIZE, ZMQ_SNDMORE);
		assert(rc == REV_BUF_SIZE);
		pos += REV_BUF_SIZE;
	}

  rc = zmq_send (sc, data.data()+pos, data.length()-pos, 0);

}


inline void my_msg_recv(void *sb, std::string &data) {

	static char rev_buf[REV_BUF_SIZE];

	int rcvmore=true;
	size_t sz = sizeof (rcvmore);
	int rc=0;

	data="";
	while (rcvmore) {
		bzero(rev_buf,REV_BUF_SIZE);
		rc = zmq_recv(sb, rev_buf, REV_BUF_SIZE, 0);
		assert (rc != -1);
		// no '\0' while be recieved
		data = data + std::string(rev_buf,rc);
		rc = zmq_getsockopt (sb, ZMQ_RCVMORE, &rcvmore, &sz);
	}
}




//假设是  单线程访问 
class channel_pool { 
	private:
		std::map<std::string,void * >::iterator dbit;
		std::map<std::string,void * > _channel_pool;
		std::string _channel_uri;
		void *ctx;
		int rc ;
		bool load;
	public:
		void init() {
			load = false;
			rc = 0;
		  _channel_uri = "ipc:///tmp/fanglf_zero_ipc";

			/*
			leveldb::Options options;	
			options.create_if_missing = true;	
			options.error_if_exists = false;	
			status = leveldb::DB::Open(options, "config", &db);
			if (!status.ok()) {
				_channel_uri = "ipc:///tmp/fanglf_zero_ipc";
			} else {
				status = db->Get(leveldb::ReadOptions(), "channel_uri", &_channel_uri);		
				if (!status.ok())
					_channel_uri = "ipc:///tmp/fanglf_zero_ipc";
			}
			delete db;
			db = NULL;
			*/
		}

		void* getChannelByName(std::string chname) {
			dbit=_channel_pool.find(chname);
			if (dbit == _channel_pool.end()) {

				if (!load) {
					ctx = zmq_init (1);
					assert (ctx);
					load = true;
				}

				void *sb = zmq_socket (ctx, ZMQ_REQ);
				assert (sb);
				int rc = zmq_connect(sb, "ipc:///tmp/fanglf_zero_ipc");
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

channel_pool _pool;

Handle<Value> Router(const Arguments& args) {

	  HandleScope scope;
		if (args.Length() < 2) {
			ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
			return scope.Close(Undefined());
		}

		v8::String::AsciiValue chname(args[0]); 
		v8::String::AsciiValue value(args[1]); 
		void *ch = _pool.getChannelByName(*chname); 
		std::string ret="0";
		my_msg_send(ch,*value);
		my_msg_recv(ch,ret);

		return scope.Close(String::New(ret.c_str()));
}


/*
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
		db = _pool.getChannelByName(*dbname);
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
		db = _pool.getChannelByName(*dbname);
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
	  target->Set(String::NewSymbol("route"),
				      FunctionTemplate::New(Router)->GetFunction());
		_pool.init();
}

NODE_MODULE(zmq_addon, Init)
