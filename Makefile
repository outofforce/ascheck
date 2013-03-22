.PHONY: subs addon cc
all: subs


subs:
	if test -d build ; then node-gyp build ; else node-gyp configure ; fi;
	cd src;make;cd ..

addon:
	if test -d build ; then node-gyp build ; else node-gyp configure ; fi;

cc:
	cd src;make;cd ..

clean:
	node-gyp clean 
	cd src;rm *.o;rm zmq_server zmq_client tcp_client;
	cd ..


