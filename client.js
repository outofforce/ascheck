var net = require('net');
var util = require('util');

var msg = 0;
var start = Date.now();
var MAX_TEST = 500000
var du =0;
function getMsg() {
	msg++;
	return ''+msg
}	

var client = net.connect({port: 3000},function() { //'connect' listener
		console.log('client connected');
		client.write(getMsg());
		});

client.on('data', function(data) {
		//console.log('recieve:'+data);
		
		if (data == '1') {
		   du++;
		}

		if (msg>=MAX_TEST) {
      var delta;
      delta = Date.now() - start;
      console.log('Completed %d in %d ms, du=%d', MAX_TEST,delta,du);
      console.log('%s transaction per second', Math.floor(MAX_TEST * 1000 / delta));
			client.end();
		}
		else 
			client.write(getMsg());
		});

client.on('end', function() {
		console.log('client disconnected');
		});
