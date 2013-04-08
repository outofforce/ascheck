var net = require('net');
var util = require('util');
var db = require('./build/Release/asleveldb');
var cluster = require('cluster');
var fs = require('fs');
var assert = require('assert');



var workers = {};
var indexs = {};
if (cluster.isMaster) {

	var startport = 3000,count = 1,begin = 1,dbpath=process.cwd();
	process.argv.forEach(function (val, index, array) {
			//console.log(index + ': ' + val);
			if (val == "-n") 
				count = process.argv[index+1];
			else if (val == "-p") 
				startport= process.argv[index+1];
			else if (val == "-b")
				begin = process.argv[index+1];
			else if (val == "-d") {
				dbpath = process.argv[index+1];
				if (fs.existsSync(dbpath) == false) {
					console.log("dbpath (%s) is not exist",dbpath);
					assert(false);
				}
			}
		});
	


	console.log('count = ' +count  + ' begin = '+begin);

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
			var t_index = indexs[worker.process.pid];
			delete indexs[worker.process.pid];
			delete workers[worker.process.pid];
			worker = cluster.fork({'worker_index':t_index,'db_path_pre':dbpath});
			workers[worker.process.pid] = worker;
			indexs[worker.process.pid] = t_index;
			}

			});

	for (var i=0;i<count;i++) {
		var worker = cluster.fork({'worker_index':Number(i)+Number(begin),'db_path_pre':dbpath,'start_port':startport});
		console.log("worker "+ worker.process.pid +" success!");
		workers[worker.process.pid] = worker;
		indexs[worker.process.pid] = Number(i)+Number(begin);
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

	server.listen(Number(start_port)+Number(index), function() { 
			console.log('server %d start at port %d ....',index,Number(start_port)+Number(index));
			db.dbinit(db_path_pre,index);
			});

}

process.on('SIGTERM',function() {
	for (var item in workers) {
		process.kill(item.pid)
	}
});
