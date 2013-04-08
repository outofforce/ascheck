var util = require('util');
var crypto = require('crypto');
var net = require('net');
var db = require('./build/Release/asleveldb');

var msg = 0;
var MAX_TEST = 100000 
var du =0;
function getMsg() {
	msg++;
	return ''+msg
}	


function md5(msg) {
	var md5sum = crypto.createHash('md5');
	return md5sum.update(msg).digest('hex');
}

function bench() { //'connection' listener
	console.log('bench begin');

	var start = Date.now();
	for (;msg<MAX_TEST;) {
		if (db.add(getMsg(),'1','djd') == 1)
			du++;
	}
	var delta;
	delta = Date.now() - start;
	console.log('Completed %d in %d ms, du=%d', MAX_TEST,delta,du);
	console.log('%s transaction per second', Math.floor(MAX_TEST * 1000 / delta));
}

function query() { //'connection' listener
	var temp = db.query("fanglf","abc");
	if (temp.ok == '1') {
		console.log('insert');
		db.add("fanglf","good baba!","abc");
	} else {
		console.log('read:',temp.v);
	}
}

db.dbinit("/tmp","");
function init_config() {
	// 设置配置数据 
	 
	 db.write("ASCHECK_ADDR_1",'127.0.0.1','config');
	 db.write("ASCHECK_PORT_1",'3001','config');
	 db.write("ASCHECK_ADDR_2",'127.0.0.1','config');
	 db.write("ASCHECK_PORT_2",'3002','config');
	 db.write("ASCHECK_ADDR_3",'127.0.0.1','config');
	 db.write("ASCHECK_PORT_3",'3003','config');
	
	 db.write("SERVICE_COUNT",'1','config');
}
function query_config() {
	// 设置配置数据 
	 
	 var r = db.query("ASCHECK_ADDR_1",'config');
	 console.log("ASCHECK_ADDR_1=",r.v);
	 r = db.query("ASCHECK_PORT_1",'config');
	 console.log("ASCHECK_PORT_1=",r.v);
	 r = db.query("ASCHECK_ADDR_2",'config');
	 console.log("ASCHECK_ADDR_2=",r.v);
	 r = db.query("ASCHECK_PORT_2",'config');
	 console.log("ASCHECK_PORT_2=",r.v);
	
	 r = db.query("SERVICE_COUNT",'config');
	 console.log("SERVICE_COUNT=",r.v);
}


init_config()
query_config()

//query_zmq()

//bench();
