var net = require('net');
var util = require('util');
var crypto = require('crypto');
var db = require('./build/Release/addon');
var cluster = require('cluster');
var os = require('os');

var numCPUs = os.cpus().length;

function md5(msg) {
	var md5sum = crypto.createHash('md5');
	return md5sum.update(msg).digest('hex');
}


var workers = {};

if (cluster.isMaster) {
	cluster.on('death',function (worker) {
		delete workers[worker.pic];
		worker = cluster.fork();
		workers[worker.pid] = worker;
	}); 
	for (var i=0;i<numCPUs;i++) {
		var worker = cluster.fork();
		workers[worker.pid] = worker;
	}
} else {

	var server = net.createServer(function(c) { 
			console.log('client connected');

			c.on('end', function() {
				console.log('client disconnected');
				});
			c.on('data',function(data) {
				c.write(''+db.add(data,"1","ghi"))  ;
				//c.write(db.route("0",data));
				});
			});


	server.on('connection', function (x) {
			console.log("total_connection:"+server.connections+ " remotePort:"+x.remotePort +" "+"remoteAddress:"+x.remoteAddress);
			});


/*
	server.on('error', function (e) {
			if (e.code == 'EADDRINUSE') 
			console.log('Address in use, retrying...');
			});
			*/

	server.listen(3000, function() { 
			console.log('server bound');
			});

}

process.on('SIGTERM',function() {
	for (var pid in workers) {
		process.kill(pid)
	}
});
