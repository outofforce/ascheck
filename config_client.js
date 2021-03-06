var net = require('net');
var util = require('util');
var fs= require('fs');
var astool= require('./astools.js')

var config_cache = {};
var config_array = new Array();

var conf_file = '';
process.argv.forEach(function (val, index, array) {
	if (val== "-c") {
		conf_file = process.argv[index+1];
		if (fs.existsSync(conf_file) == false) {
			console.log("conf_file (%s) is not exist",dbpath);
			assert(false);
		}
	}
});

if (conf_file == '') assert(false);


prase_config_file();

function prase_config_file() {
	var content;
	content = fs.readFileSync(conf_file);
	var doc = content.toString();
	var end = 0;
	var begin = 0;

	while (true) {
		end = doc.indexOf('\n',begin);
		if (end == -1) break;
		var item = doc.substring(begin,end);
		var data = astool.split(item,' ');
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
				var tmp = astool.split(id_rang,'..');
				if (tmp[0] == 1) continue;
				id_begin=tmp[1];
				id_end=tmp[2];
				tmp = astool.split(data[2],':');
				if (tmp[0] == 1) continue;
				ip = tmp[1];
				port_begin=tmp[2];
				//console.log(id_begin+'-'+id_end+'-'+ip+'-'+port_begin);
				var index=0;
				for (var j=Number(id_begin);j<=Number(id_end);j++) {
					config_cache['ASCHECK_ADDR_'+j]=ip+':'+(Number(port_begin)+Number(index));
					config_array.push('ASCHECK_ADDR_'+j);
					index++;
				}
			} else {
				config_cache[data[1]]=data[2];
				config_array.push(data[1]);
			}
		}
		begin = end+1;
	}
	//console.log(config_cache);
}




var send_index=0;
var s;
var client = net.connect({port: 3000},function() { //'connect' listener
		console.log('connect config_server sucess!');
		s=astool.makeProto(
				config_array[send_index],
				config_cache[config_array[send_index]]);

		//console.log('Send :'+s);
		client.write(s);
		});


client.on('data', function(data) {
		
		if (Number(data.toString()) >= 0)
			console.log('SET '+config_array[send_index]+'='+ config_cache[config_array[send_index]] +' SUCCESS!');
		else
			console.log('SET '+config_array[send_index]+' FAIL!!');

		send_index++;
		if (send_index >= config_array.length)
		{
			client.end();
			return ;
		}
	
		s=astool.makeProto(
				config_array[send_index],
				config_cache[config_array[send_index]]);

		//console.log('Send :'+s);
		client.write(s);

		});

client.on('end', function() {
		console.log('client disconnected');
		});
