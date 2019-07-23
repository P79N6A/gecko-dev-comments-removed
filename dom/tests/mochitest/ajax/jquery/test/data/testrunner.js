var _config = {
	fixture: null,
	Test: [],
	stats: {
		all: 0,
		bad: 0
	},
	queue: [],
	blocking: true,
	timeout: null,
	expected: null,
	currentModule: null,
	asyncTimeout: 2 
};

$(function() {
	$('#userAgent').html(navigator.userAgent);
	runTest();	
});

function synchronize(callback) {
	_config.queue[_config.queue.length] = callback;
	if(!_config.blocking) {
		process();
	}
}

function process() {
	while(_config.queue.length && !_config.blocking) {
		var call = _config.queue[0];
		_config.queue = _config.queue.slice(1);
		call();
	}
}

function stop(allowFailure) {
	_config.blocking = true;
	var handler = allowFailure ? start : function() {
		ok( false, "Test timed out" );
		start();
	};
	_config.timeout = setTimeout(handler, _config.asyncTimeout * 1000);
}
function start() {
	if(_config.timeout)
		clearTimeout(_config.timeout);
	_config.blocking = false;
	process();
}

function runTest() {
	_config.blocking = false;
	var time = new Date();
	_config.fixture = document.getElementById('main').innerHTML;
	synchronize(function() {
		time = new Date() - time;
		$("<div>").html(['<p class="result">Tests completed in ',
			time, ' milliseconds.<br/>',
			_config.stats.bad, ' tests of ', _config.stats.all, ' failed.</p>']
			.join(''))
			.appendTo("body");
		$("#banner").addClass(_config.stats.bad ? "fail" : "pass");
		if ( parent.runAJAXTest )
			parent.runAJAXTest();
	});
}

function test(name, callback, nowait) {
	if(_config.currentModule)
		name = _config.currentModule + " module: " + name;
		
	var filter = location.search.slice(1);
	if ( filter && encodeURIComponent(name) != filter )
		return;
		
	synchronize(function() {
		_config.Test = [];
		try {
			callback();
		} catch(e) {
			if( typeof console != "undefined" && console.error && console.warn ) {
				console.error("Test " + name + " died, exception and test follows");
				console.error(e);
				console.warn(callback.toString());
			}
			_config.Test.push( [ false, "Died on test #" + (_config.Test.length+1) + ": " + e ] );
		}
	});
	synchronize(function() {
		reset();
		
		
		if(nowait) return;
		
		if(_config.expected && _config.expected != _config.Test.length) {
			_config.Test.push( [ false, "Expected " + _config.expected + " assertions, but " + _config.Test.length + " were run" ] );
		}
		_config.expected = null;
		
		var good = 0, bad = 0;
		var ol = document.createElement("ol");
		ol.style.display = "none";
		var li = "", state = "pass";
		for ( var i = 0; i < _config.Test.length; i++ ) {
			var li = document.createElement("li");
			li.className = _config.Test[i][0] ? "pass" : "fail";
			li.innerHTML = _config.Test[i][1];
			ol.appendChild( li );
			
			_config.stats.all++;
			if ( !_config.Test[i][0] ) {
				state = "fail";
				bad++;
				_config.stats.bad++;
			} else good++;

			if ( parent.SimpleTest )
				parent.SimpleTest.ok( _config.Test[i][0], name, _config.Test[i][1] );
		}
	
		var li = document.createElement("li");
		li.className = state;
	
		var b = document.createElement("strong");
		b.innerHTML = name + " <b style='color:black;'>(<b class='fail'>" + bad + "</b>, <b class='pass'>" + good + "</b>, " + _config.Test.length + ")</b>";
		b.onclick = function(){
			var n = this.nextSibling;
			if ( jQuery.css( n, "display" ) == "none" )
				n.style.display = "block";
			else
				n.style.display = "none";
		};
		$(b).dblclick(function(event) {
			var target = jQuery(event.target).filter("strong").clone();
			if ( target.length ) {
				target.children().remove();
				location.href = location.href.match(/^(.+?)(\?.*)?$/)[1] + "?" + encodeURIComponent($.trim(target.text()));
			}
		});
		li.appendChild( b );
		li.appendChild( ol );
	
		document.getElementById("tests").appendChild( li );		
	});
}


function module(moduleName) {
	_config.currentModule = moduleName;
}




function expect(asserts) {
	_config.expected = asserts;
}




function reset() {
	document.getElementById('main').innerHTML = _config.fixture;
}





function ok(a, msg) {
	_config.Test.push( [ !!a, msg ] );
}




function isSet(a, b, msg) {
	var ret = true;
	if ( a && b && a.length != undefined && a.length == b.length ) {
		for ( var i = 0; i < a.length; i++ )
			if ( a[i] != b[i] )
				ret = false;
	} else
		ret = false;
	if ( !ret )
		_config.Test.push( [ ret, msg + " expected: " + serialArray(b) + " result: " + serialArray(a) ] );
	else 
		_config.Test.push( [ ret, msg ] );
}




function isObj(a, b, msg) {
	var ret = true;
	
	if ( a && b ) {
		for ( var i in a )
			if ( a[i] != b[i] )
				ret = false;

		for ( i in b )
			if ( a[i] != b[i] )
				ret = false;
	} else
		ret = false;

    _config.Test.push( [ ret, msg ] );
}

function serialArray( a ) {
	var r = [];
	
	if ( a && a.length )
        for ( var i = 0; i < a.length; i++ ) {
            var str = a[i].nodeName;
            if ( str ) {
                str = str.toLowerCase();
                if ( a[i].id )
                    str += "#" + a[i].id;
            } else
                str = a[i];
            r.push( str );
        }

	return "[ " + r.join(", ") + " ]"
}






function q() {
	var r = [];
	for ( var i = 0; i < arguments.length; i++ )
		r.push( document.getElementById( arguments[i] ) );
	return r;
}






function t(a,b,c) {
	var f = jQuery(b);
	var s = "";
	for ( var i = 0; i < f.length; i++ )
		s += (s && ",") + '"' + f[i].id + '"';
	isSet(f, q.apply(q,c), a + " (" + b + ")");
}










function url(value) {
	return value + (/\?/.test(value) ? "&" : "?") + new Date().getTime() + "" + parseInt(Math.random()*100000);
}













function equals(expected, actual, message) {
	var result = expected == actual;
	message = message || (result ? "okay" : "failed");
	_config.Test.push( [ result, result ? message + ": " + expected : message + " expected: " + expected + " actual: " + actual ] );
}









function triggerEvent( elem, type, event ) {
	if ( jQuery.browser.mozilla || jQuery.browser.opera ) {
		event = document.createEvent("MouseEvents");
		event.initMouseEvent(type, true, true, elem.ownerDocument.defaultView,
			0, 0, 0, 0, 0, false, false, false, false, 0, null);
		elem.dispatchEvent( event );
	} else if ( jQuery.browser.msie ) {
		elem.fireEvent("on"+type);
	}
}
