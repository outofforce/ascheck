
taget = libascheck.so test
all: $(taget) 

libascheck.so:ascheck.h ascheck.cc 
	$(CXX) $(CXXFLAGS) -fPIC -shared -o  libascheck.so ascheck.cc  -I/usr/local/include -L/usr/local/lib  -lcrypto -Wno-deprecated-declarations

test: test.cc libascheck.so
	$(CXX) $(CXXFLAGS) -o test test.cc -I../include -I/usr/local/include -L/usr/local/lib -lzmq -L. -lascheck



clean:
	rm -f *.o
	rm -f $(taget)


