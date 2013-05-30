var net = require('net');
var util = require('util');
var crypto = require('crypto');
var db = require('./build/Release/asleveldb');
var fs = require('fs');
var assert = require('assert');
var astool = require('./astools.js');

var config_path=process.cwd(); //default config db path
var port=3000; // default config_server port
var dbname='config'; // default config db name
var index_list='1'; // default config db name
var indexs = {};

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

					var tp = astool.praseProto(data);
					// modify config db, part must be config; 
					tp['part']=dbname;
					if (tp['op'] == 4) {
						// check
						c.write('0'+db.write(tp['key'],tp['value'],tp['part'])+'\r\n');

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
