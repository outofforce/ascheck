var net = require('net');
var util = require('util');
var db = require('./build/Release/asleveldb');
var cluster = require('cluster');
var fs = require('fs');
var assert = require('assert');
var EventEmitter= require('events').EventEmitter;



var config_ready_event = new EventEmitter();

var workers = {};
var indexs_work = {};
var serv_addrs = {};
//var indexs = {};

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

function prase_config_arg(in_list) {
	var content;
	var end = 0;
	var begin = 0;

	var item = in_list;
	var i=0;
	var tmp_indexs={};
	var indexs = new Array();
	while (true) {
		end = item.indexOf(',',begin);
		if (end == -1) { tmp_indexs[i]=item; break; }
		else {
			tmp_indexs[i]=item.substring(0,end);
			item=item.substring(end+1,item.length);
		}
		i++;
	}

	i=0;
  for (var it in tmp_indexs) {
		    var data=split(tmp_indexs[it],'..');
				if (data[0]==1) {
					indexs[i]=tmp_indexs[it];
					i++;
				} else {
					begin=data[1];
					end=data[2];
					for (var j=Number(begin);j<=Number(end);j++) {
						indexs[i]=j;
						i++;
					}
				}
	}

	//console.log(indexs);
	return indexs;
}

function makeQueryProto(key) {

	var str="0200";
	if (String(key).length > 9)
		str = str + String(key).length.toString();
	else
		str = str + '0'+ String(key).length.toString();
	str = str+'0000';


	str = str+String(key)+'\r\n';

	return str;
}


function fetch_from_config(s_indexs) {
	var c = net.createConnection(3000);
	var data='';
	var now_index=0;
	c.on('data', function(buf) {
		data += buf.toString();
		if (data.indexOf('\r\n') != -1) {
			var ret_status = Number(data.substring(0,2));
			if (ret_status == '00') {
				var value= data.substring(2,data.length-2);
				
				var tmp_var = split(value,':');
				if (tmp_var[0] != 0) {
					assert(false,'ASCHECK_ADDR_' + s_indexs[now_index]+' config error !');
				}
				serv_addrs['ASCHECK_ADDR_' + s_indexs[now_index]] = tmp_var[2];
				now_index++;
				if (now_index == s_indexs.length) {
					c.end();
				} else {
					c.write(makeQueryProto('ASCHECK_ADDR_' + s_indexs[now_index]));
				}
			} else {
				assert(false,'ASCHECK_ADDR_' + s_indexs[now_index] + ' undefine in config server');
			}

			data='';
		}

	});

	c.on('connect', function() {
		c.write(makeQueryProto('ASCHECK_ADDR_' + s_indexs[now_index]));
	});

	c.on('close', function() {
		config_ready_event.emit('readygo');
	});

}


var startport = 3000,count = 1,dbpath=process.cwd();
var index_str = '1';

process.argv.forEach(function (val, index, array) {
	//console.log(index + ': ' + val);
	if (val == "-n") 
	count = process.argv[index+1];
	else if (val == "-p") 
	startport= process.argv[index+1];
	else if (val == "-i")
	index_str = process.argv[index+1];
	else if (val == "-d") {
		dbpath = process.argv[index+1];
		if (fs.existsSync(dbpath) == false) {
			console.log("dbpath (%s) is not exist",dbpath);
			assert(false);
		}
	}
});

var serv_array = prase_config_arg(index_str);

fetch_from_config(serv_array);

config_ready_event.on('readygo',function() {

if (cluster.isMaster) {



	cluster.on('exit', function(worker, code, signal) {
			console.log('worker ' + worker.process.pid + ' died');
			if( signal ) {
			console.log("worker was killed by signal: "+signal);
			} else if( code !== 0 ) {
			console.log("worker exited with error code: "+code);
			} else {
			console.log("worker exit normal!");
			}
			console.log("restart this workder...");
			if (code != 1) {
			var t_index = indexs_work[worker.process.pid];
			delete indexs_work[worker.process.pid];
			delete workers[worker.process.pid];
			worker = cluster.fork({'worker_index':t_index,'db_path_pre':dbpath});
			workers[worker.process.pid] = worker;
			indexs_work[worker.process.pid] = t_index;
			}

			});

	for (var i=0;i<serv_array.length;i++) {
		var worker = cluster.fork({'worker_index':serv_array[i],'db_path_pre':dbpath,'start_port':serv_addrs['ASCHECK_ADDR_' + serv_array[i]]});
		console.log("worker "+ worker.process.pid +" success!");
		workers[worker.process.pid] = worker;
		indexs_work[worker.process.pid] = serv_array[i] ;
	}

} else {


	var index = process.env['worker_index'];
	var db_path_pre = process.env['db_path_pre'];
	var start_port= process.env['start_port'];

	var server = net.createServer(function(c) { //'connection' listener
			console.log('client connected');

			c.on('end', function() {
				console.log('client disconnected');
				});

			var data  = '';
			c.on('data',function(buf) {
				
				data += buf.toString();
				//console.log(buf);
				//console.log(data.toString());
				if (data.indexOf('\r\n') != -1) { 
					//console.log("in HERE");

					var key,value,part,begin,end;
					var op = Number(data.substring(0,2));
					var partlen = data.substring(2,4);
					var keylen = data.substring(4,6);

					begin=10;
					end=10+Number(partlen);

					//console.log('----- end = '+end);
					part = data.substring(begin,end);

					begin = end;
					end=end+Number(keylen);
					key= data.substring(begin,end);

					if (op == 1) {
						var valuelen = data.substring(6,10);
						begin=end;
						end=end+Number(valuelen);
						value= data.substring(begin,end);
						//console.log('1');
						//console.log('----- '+part+ ' ---- ' + key+ ' --- ' + value+ ''); 
						c.write('0'+db.add(key,value,part)+'\r\n');
					} else if (op == 2) {

						//console.log('2');
						var temp = db.query(key,part);
						if (temp.ok == 1) {
							c.write('01'+'\r\n');
						} else {
							c.write('00'+temp.v+'\r\n');
						}
					} else if (op == 3) {
						//console.log('3');
						c.write('0'+db.del(key,part)+'\r\n');
					}
					data = '';
				}

				//console.log('4');
				
			});
	});



	server.on('connection', function (x) {
			console.log('server(%d) total_connection:%d remotePort:%d remoteAddress:%s',
			Number(index),server.connections,x.remotePort,x.remoteAddress);
			});

	server.listen(Number(start_port), function() { 
			console.log('server %d start at port %d ....',index,Number(start_port));
			db.dbinit(db_path_pre,index);
			});

}
});


process.on('SIGTERM',function() {
	for (var item in workers) {
		process.kill(workers[item].process.pid);
	}
});
