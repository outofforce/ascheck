
exports.split = function(doc,dem) {
	var pos = 0;
	pos = doc.indexOf(dem);
	var pair={};
	if (pos == -1) { pair[0]=1; return pair; }
	pair[0]= 0;
	pair[1]= doc.substring(0,pos);
	pair[2]= doc.substring(pos+dem.length,doc.length);
	return pair;
};


exports.makeQueryProto=function (key) {
	var str="0200";
	if (String(key).length > 9)
		str = str + String(key).length.toString();
	else
		str = str + '0'+ String(key).length.toString();
	str = str+'0000';

	str = str+String(key)+'\r\n';
	return str;
};

exports.prase_config_arg = function(in_list) {
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
		var data=exports.split(tmp_indexs[it],'..');
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

	return indexs;
};


exports.praseRet = function(data) {
	var tp = {};
	tp['ret']= Number(data.substring(0,2));
  if (tp['ret'] == 0) {
		tp['value'] = data.substring(2,data.length-2);
	}
	return tp;
};

exports.praseProto = function(data) {
	var tmp_proto = {};
	tmp_proto['op']=-1;
	var key,value,part,begin,end;
	var op = Number(data.substring(0,2));
	var partlen = data.substring(2,4);
	var keylen = data.substring(4,6);

	begin=10;
	end=10+Number(partlen);
	//console.log('----- end = '+end);
	//get op
	tmp_proto['op']=op;

	// get port
	part = data.substring(begin,end);
	tmp_proto['part']=part;

	// get key
	begin = end;
	end=end+Number(keylen);
	key= data.substring(begin,end);
	tmp_proto['key']=key;

	if (op == 1 || op == 4) {
		// add( check dup)  or set has value  
		var valuelen = data.substring(6,10);
		begin=end;
		end=end+Number(valuelen);
		value= data.substring(begin,end);
		tmp_proto['value']=value;

	} 

	return tmp_proto;

};



exports.makeProto=function(key,value) {
	// how too
	// very aweful
	var str="0400";
	if (String(key).length > 9)
		str = str + String(key).length.toString();
	else 
		str = str + '0'+ String(key).length.toString();
		
	if (String(value).length >999)
		str = str + String(value).length.toString();
	else if (String(value).length >99)
		str = str + '0'+ String(value).length.toString();
	else if (String(value).length >9)
		str = str + '00'+ String(value).length.toString();
	else
		str = str + '000'+ String(value).length.toString();

	str = str+String(key)+String(value)+'\r\n';
	
	return str;
};



