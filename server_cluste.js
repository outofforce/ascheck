var net = require('net');
var util = require('util');
var db = require('./build/Release/asleveldb');
var cluster = require('cluster');
var fs = require('fs');
var assert = require('assert');
var EventEmitter= require('events').EventEmitter;
var astool= require('./astools.js')

var config_ready_event = new EventEmitter();

var workers = {};
var indexs_work = {};
var serv_addrs = {};

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

var serv_array = astool.prase_config_arg(index_str);


function fetch_from_config(s_indexs) {
	var c = net.createConnection(3000);
	var data='';
	var now_index=0;
	c.on('data', function(buf) {
		data += buf.toString();
		if (data.indexOf('\r\n') != -1) {

			var tp = astool.praseRet(data);
			if (tp['ret'] == 0 ) {
				var value= tp['value'];

				var tmp_var = astool.split(value,':');
				if (tmp_var[0] != 0) {
					assert(false,'ASCHECK_ADDR_' + s_indexs[now_index]+' config error !');
				}
				serv_addrs['ASCHECK_ADDR_' + s_indexs[now_index]] = tmp_var[2];
				now_index++;
				if (now_index == s_indexs.length) {
					c.end();
				} else {
					c.write(astool.makeQueryProto('ASCHECK_ADDR_' + s_indexs[now_index]));
				}
			} else {
				assert(false,'ASCHECK_ADDR_' + s_indexs[now_index] + ' undefine in config server');
			}

			data='';
		}

	});

	c.on('connect', function() {
		c.write(astool.makeQueryProto('ASCHECK_ADDR_' + s_indexs[now_index]));
	});

	c.on('close', function() {
		config_ready_event.emit('readygo');
	});

}


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
			// todo  code = 8  ;
			if (code != 1) {
				var t_index = indexs_work[worker.process.pid];
				delete indexs_work[worker.process.pid];
				delete workers[worker.process.pid];

				worker = cluster.fork({'worker_index':t_index,'db_path_pre':dbpath,'start_port':serv_addrs['ASCHECK_ADDR_' + t_index]});
				workers[worker.process.pid] = worker;
				indexs_work[worker.process.pid] = t_index;
			}

		});

		for (var i=0;i<serv_array.length;i++) {
			var worker = cluster.fork(
					{ 'worker_index':serv_array[i],
						'db_path_pre':dbpath,
				'start_port':serv_addrs['ASCHECK_ADDR_' + serv_array[i]]});
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

					var tp = astool.praseProto(data);
					if (tp['op'] == 1) {
						// check
						c.write('0'+db.add(tp['key'],tp['value'],tp['part'])+'\r\n');

					} else if (tp['op'] == 2)  {
						// query
						var temp = db.query(tp['key'],tp['part']);
						if (temp.ok == 1) {
							c.write('01'+'\r\n');
						} else {
							c.write('00'+temp.v+'\r\n');
						}

					} else if (tp['op'] == 3)  {
						// del
						c.write('0'+db.del(tp['key'],tp['part'])+'\r\n');

					} 

					data="";

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
