
taget = zmq_server zmq_client tcp_client tcp_server local_dbtest pp simple redis_client 
all: $(taget) 

tcp_client.o:tcp_client.cc 
	$(CXX) $(CXXFLAGS) -c tcp_client.cc


zmq_server.o:zmq_server.cc ../include/tools.h
	$(CXX) $(CXXFLAGS) -I/user/local/include -I../include -I/home/fanglf/src/leveldb-1.9.0/include -c zmq_server.cc

zmq_client.o:zmq_client.cc ../include/tools.h
	$(CXX) $(CXXFLAGS) -I/user/local/include -I../include -I/home/fanglf/src/leveldb-1.9.0/include -c zmq_client.cc

tcp_client:tcp_client.o 
	$(CXX) $(CXXFLAGS) tcp_client.o -o tcp_client  -I../include


zmq_client:zmq_client.o 
	$(CXX) $(CXXFLAGS) -L/usr/local/lib -lzmq zmq_client.o -o zmq_client

zmq_server:zmq_server.o
	$(CXX) $(CXXFLAGS) -L/usr/local/lib -lzmq -L/home/fanglf/src/leveldb-1.9.0 -lleveldb zmq_server.o -o zmq_server

tcp_server:tcp_server.cc 
	$(CXX) $(CXXFLAGS) tcp_server.cc -o tcp_server -I../include -I/usr/local/include -I/home/fanglf/src/leveldb-1.9.0/include  -lpthread -L/home/fanglf/src/leveldb-1.9.0 -lleveldb -L/usr/local/lib -lzmq

local_dbtest:local_dbtest.cc 
	$(CXX) $(CXXFLAGS) local_dbtest.cc -o local_dbtest -I../include -I/home/fanglf/src/leveldb-1.9.0/include  -L/home/fanglf/src/leveldb-1.9.0 -lleveldb 


pp:zmq_pp.cc 
	$(CXX) $(CXXFLAGS) zmq_pp.cc -o pp -I../include -I/usr/local/include -I/home/fanglf/src/leveldb-1.9.0/include  -lpthread -L/home/fanglf/src/leveldb-1.9.0 -lleveldb -L/usr/local/lib -lzmq 

simple:simple_server.cc 
	$(CXX) $(CXXFLAGS) -D_REENTRANT  simple_server.cc -o simple -I../include -I/usr/local/include -I/home/fanglf/src/leveldb-1.9.0/include  -lpthread -L/home/fanglf/src/leveldb-1.9.0 -lleveldb -L/usr/local/lib -lzmq -L. -lunp

redis_client:redis_client.cc 
	$(CXX) $(CXXFLAGS) redis_client.cc -o redis_client -I../include -I/usr/local/include -L/usr/local/lib -lhiredis


clean:
	rm -f *.o
	rm -f $(taget)


