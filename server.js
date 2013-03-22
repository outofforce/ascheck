var net = require('net');
var util = require('util');
var crypto = require('crypto');
var db = require('./build/Release/zmq_addon');

function md5(msg) {
	var md5sum = crypto.createHash('md5');
	return md5sum.update(msg).digest('hex');
}

var server = net.createServer(function(c) { //'connection' listener
		console.log('client connected');

		c.on('end', function() {
			console.log('client disconnected');
			});
		c.on('data',function(data) {
			//console.log('Client ='+data);
			//c.write(''+db.add(data,"1","ghi"))  ;
			c.write(db.route("0",data))  ;
			//c.write(md5(data))
			//c.write('0');
			});
		//c.pipe(c);
		});


server.on('connection', function (x) {
		console.log("total_connection:"+server.connections+ " remotePort:"+x.remotePort +" "+"remoteAddress:"+x.remoteAddress);
		});


server.on('error', function (e) {
		if (e.code == 'EADDRINUSE') 
			console.log('Address in use, retrying...');
		});

server.listen(3000, function() { 
		console.log('server bound');
});
