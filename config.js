var util = require('util');
var crypto = require('crypto');
var net = require('net');
var db = require('./build/Release/asleveldb');
var assert = require('assert');
var fs = require('fs');


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

function split(doc,dem) {
	var pos = 0;
	pos = doc.indexOf(dem);
	var pair={};
	if (pos == -1) { pair[0]=1; return pair; }
	pair[0]= 0;
	pair[1]= doc.substring(0,pos);
	pair[2]= doc.substring(pos+dem.length,doc.length);
	return pair;
}

function oo() {
	var content;
	content = fs.readFileSync('./ascheck.conf');
	var doc = content.toString();
	var end = 0;
	var begin = 0;

	var config_cache = {};
	var config_array = new Array();
	while (true) {
		end = doc.indexOf('\n',begin);
		if (end == -1) break;
		var item = doc.substring(begin,end);
		
		var data = split(item,' ');
		if (data[0] == 0) {
			if (data[1].indexOf('SERVICE_COUNT') != -1) {
				// SERVICE_COUNT
				config_cache[data[1]]=data[2];
				config_array.push(data[1]);

			} else if (data[1].indexOf('ASCHECK_ADDR_') != -1) {
				// ASCHECK_ADDR
				config_cache[data[1]]=data[2];
				config_array.push(data[1]);
			} else if (data[1].indexOf('SERV_ADDR_RANG_') != -1) {
				// RANGE
				var id_begin,id_end,ip,port_begin;
				var id_rang = data[1].substring(
															'SERV_ADDR_RANG_'.length,data[1].length);
				var tmp = split(id_rang,'..');
				if (tmp[0] == 1) continue;
				id_begin=tmp[1];
				id_end=tmp[2];
				tmp = split(data[2],':');
				if (tmp[0] == 1) continue;
				ip = tmp[1];
				port_begin=tmp[2];
				//console.log(id_begin+'-'+id_end+'-'+ip+'-'+port_begin);
				var index=0;
				for (var j=Number(id_begin);j<Number(id_end);j++) {
					config_cache['ASCHECK_ADDR_'+j]=ip+':'+(Number(port_begin)+Number(index));
					config_array.push('ASCHECK_ADDR_'+j);
					index++;
				}
			}
		}
		begin = end+1;
	}
	console.log(config_cache);

	for (var i=0;i<config_array.length;i++) {
		console.log(config_array[i]);
	}
}

oo();
//init_config()
//query_config()

//query_zmq()

//bench();
