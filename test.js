var util = require('util');
var crypto = require('crypto');
var db = require('./build/Release/zmq_addon');

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
		if (db.route("0",getMsg()) == '1')
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

function query_zmq() { //'connection' listener
	console.log(db.route("0","ewewew"));
	console.log(db.route("1","ewewew"));
	console.log(db.route("2","ewewew"));
	console.log(db.route("3","ewewew"));

}

//query_zmq()

bench();
