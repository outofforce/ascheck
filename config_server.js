var net = require('net');
var util = require('util');
var crypto = require('crypto');
var db = require('./build/Release/asleveldb');
var fs = require('fs');
var assert = require('assert');

var config_path=process.cwd(); //default config db path
var port=3000; // default config_server port
var dbname='config'; // default config db name

process.argv.forEach(function (val, index, array) {
		if (val == "-p") {
			port= process.argv[index+1];
		} else if (val == "-d") {
			config_path = process.argv[index+1];
			if (fs.existsSync(config_path) == false) {
				console.log("config_path (%s) is not exist",config_path);
				assert(false);
			}
		} else if (val == "-n") {
			dbname = process.argv[index+1];
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

					var key,value,part,begin,end;
					var op = Number(data.substring(0,2));
					var partlen = data.substring(2,4);
					var keylen = data.substring(4,6);

					begin=10;
					end=10+Number(partlen);

					part = dbname;

					begin = end;
					end=end+Number(keylen);
					key= data.substring(begin,end);

					if (op == 4) { // write
						var valuelen = data.substring(6,10);
						begin=end;
						end=end+Number(valuelen);
						value= data.substring(begin,end);
						//console.log('----- '+part+ ' ---- ' + key+ ' --- ' + value+ ''); 
						c.write('0'+db.write(key,value,part)+'\r\n');
					} else if (op == 2) {
						var temp = db.query(key,part);
						if (temp.ok == 1) {
							c.write('01'+'\r\n');
						} else {
							c.write('00'+temp.v+'\r\n');
						}
					} else if (op == 3) {
						c.write('0'+db.del(key,part)+'\r\n');
					}
					data = '';
				}

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
		db.dbinit(config_path,"");
});
