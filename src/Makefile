
taget = libascheck.so test zmq_config
all: $(taget) 

libascheck.so:ascheck.h ascheck.cc 
	$(CXX) $(CXXFLAGS) -fPIC -shared -o  libascheck.so ascheck.cc  -I/usr/local/include -L/usr/local/lib -lzmq

test: test.cc libascheck.so
	$(CXX) $(CXXFLAGS) -o test test.cc -I../include -I/usr/local/include -L/usr/local/lib -lzmq -L. -lascheck

zmq_config:zmq_config.cc 
	$(CXX) $(CXXFLAGS) zmq_config.cc -o zmq_config -I../include -I/usr/local/include -I/Users/outofforce/Work/src/leveldb-1.9.0/include  -L/Users/outofforce/Work/src/leveldb-1.9.0 -lleveldb -L/usr/local/lib -lzmq 






clean:
	rm -f *.o
	rm -f $(taget)


