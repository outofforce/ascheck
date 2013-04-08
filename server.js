var net = require('net');
var util = require('util');
var crypto = require('crypto');
var db = require('./build/Release/asleveldb');
var fs = require('fs');
var assert = require('assert');

/*
function md5(msg) {
	var md5sum = crypto.createHash('md5');
	return md5sum.update(msg).digest('hex');
}
*/
var dbpath=process.cwd();
var port=3001
process.argv.forEach(function (val, index, array) {
		if (val == "-p") {
			port= process.argv[index+1];
		} else if (val == "-d") {
			dbpath = process.argv[index+1];
			if (fs.existsSync(dbpath) == false) {
				console.log("dbpath (%s) is not exist",dbpath);
				assert(false);
			}
		}
		});


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
		console.log("total_connection:"+server.connections+ " remotePort:"+x.remotePort +" "+"remoteAddress:"+x.remoteAddress);
		});


server.on('error', function (e) {
		if (e.code == 'EADDRINUSE') 
			console.log('Address in use, retrying...');
		});

server.listen(port, function() { 

		console.log('server bound on '+port);
		db.dbinit(dbpath,"");
});
