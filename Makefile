.PHONY: subs addon cc
all: subs


subs:
	if test -d build ; then node-gyp build ; else node-gyp configure ; node-gyp build; fi;
	cd src;make;cd ..

addon:
	if test -d build ; then node-gyp build ; else node-gyp configure ; fi;

cc:
	cd src;make;cd ..

clean:
	node-gyp clean 
	cd src;make clean; cd ..


