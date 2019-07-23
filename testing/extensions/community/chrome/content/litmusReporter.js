













































const VERSION = '0.0.2';















function LitmusResults(a) {
	this.machinename = a.machinename || '';
	this.requireField('username', a);
	this.requireField('password', a);
	this.server = a.server || 'http://litmus.mozilla.org/process_test.cgi';
	this.action = 'submit';
	
	this.results = new Array();
	this.logs = new Array();
}

LitmusResults.prototype = {
	
	
	requireField : function (name, args) {
		if (args[name] === undefined) {
			throw "Missing required field in Litmus result submission: "+name;
		}
		this[name] = args[name];
	},
	sysconfig : function (a) {
		this.requireField('product', a);
		this.requireField('platform', a);
		this.requireField('opsys', a);
		this.requireField('branch', a);
		this.requireField('buildid', a);
		this.requireField('locale', a);
		this.buildtype = a.buildtype;
	},
	addLog : function(log) {
		this.logs.push(log);
	},
	addResult : function(result) {
		this.results.push(result);
	},
	toXML : function() {
		var d = '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>'+"\n";
		d += '<!DOCTYPE litmusresults PUBLIC' +  
               ' "-//Mozilla Corporation//Litmus Result Submission DTD//EN/"' +
               ' "http://litmus.mozilla.org/litmus_results.dtd">'+"\n";
		d += '<litmusresults action="'+this.action+'" useragent="'+
			'litmusReporter.js/'+VERSION+' ('+this.machinename+')" '+
			(this.machinename ? 'machinename="'+this.machinename+'">' : '>') +"\n";
		d += '  <testresults username="'+this.username+'"'+"\n";
		d += '     authtoken="'+this.password+'"'+"\n";
		d += '     product="'+this.product+'"'+"\n";
		d += '     platform="'+this.platform+'"'+"\n";
		d += '     opsys="'+this.opsys+'"'+"\n";
		d += '     branch="'+this.branch+'"'+"\n";
		d += '     buildid="'+this.buildid+'"'+"\n";
		d += '     locale="'+this.locale+'"';
		if (this.buildtype) {
			d += "\n"+'     buildtype="'+this.buildtype+'"'+">\n";
		} else {
			d += ">\n";
		}
		if (this.logs) {
			if (this.logs instanceof Array) {
				for(var i=0; i<this.logs.length; i++) {
					if (this.logs[i] instanceof Log) {
						d += this.logs[i].toXML();
					}
				}
			} else {
				if (this.logs instanceof Log) {
					d += this.logs.toXML();
				}
			}
		}
		if (this.results) {
			for(var i=0; i<this.results.length; i++) {
				d += this.results[i].toXML();
			}
		}
		d += '  </testresults>'+"\n";
    	d += '</litmusresults>'+"\n";
		return d;
	},
};









function Log(a) {
	this.requireField('type', a);
	this.requireField('data', a);
}

Log.prototype = {
	type: null,
	data: null,
	requireField : function (name, args) {
		if (args[name] === undefined) {
			throw "Missing required field in Litmus result submission: "+name;
		}
		this[name] = args[name];
	},
	toXML : function () {
		var d = '<log logtype="'+this.type+'">'+"\n";
		d += '  <![CDATA['+this.data+']]>'+"\n";
		d += '</log>'+"\n";
		return d;
	},
};
	




function Result(a) {
	this.requireField('testid', a);
	this.requireField('resultstatus', a);
	this.requireField('exitstatus', a);
	this.requireField('duration', a);
	
	
	if (a.timestamp) { this.timestamp = a.timestamp; }
	else {
		var d = new Date();
		this.timestamp=''+d.getFullYear()+leadZero((d.getMonth()+1))+
			leadZero(d.getDate())+leadZero(d.getHours())+leadZero(d.getMinutes())+
			leadZero(d.getSeconds());
	}
	
	this.comment = a.comment;
	this.bugnumber = a.bugnumber;
	this.logs = a.log;
	this.automated = a.isAutomatedResult ? a.isAutomatedResult : 1;
}

Result.prototype = {
	requireField : function (name, args) {
		if (args[name] === undefined) {
			throw "Missing required field in Litmus result submission: "+name;
		}
		this[name] = args[name];
	},	
	toXML : function() {
		var d  = '<result testid="'+this.testid+'"'+"\n";
		d += '		 is_automated_result="'+this.automated+'"'+"\n";
		d += '      resultstatus="'+this.resultstatus+'"'+"\n";
		d += '      exitstatus="'+this.exitstatus+'"'+"\n";
		d += '      duration="'+this.duration+'"'+"\n";
		d += '      timestamp="'+this.timestamp+'">'+"\n";
		
		if (this.comment) {
			d += '  <comment>'+this.comment+'</comment>'+"\n";
		}
		
		if (this.bugnumber) {
			d += '  <bugnumber>'+this.bugnumber+'</bugnumber>'+"\n";
		}
		if (this.logs) {
			if (this.logs instanceof Array) {
				for(var i=0; i<this.logs.length; i++) {
					if (this.logs[i] instanceof Log) {
						d += this.logs[i].toXML();
					}
				}
			} else {
				if (this.logs instanceof Log) {
					d += this.logs.toXML();
				}
			}
		}
		d += '</result>'+"\n";
		return d;
	},
};

function leadZero(num) {
	if (num.toString().length == 1) { return '0'+num }
	return num;
}