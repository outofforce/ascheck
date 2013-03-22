all:zmq_server zmq_client tcp_client

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

clean:
	rm *.o
	rm zmq_server zmq_client tcp_client


