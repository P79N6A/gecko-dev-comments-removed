module("ajax");






if ( !isLocal ) {

test("$.ajax() - success callbacks", function() {
	expect( 8 );
	
	$.ajaxSetup({ timeout: 0 });
	
	stop();
	
	setTimeout(function(){	
        $('#foo').ajaxStart(function(){
            ok( true, "ajaxStart" );
        }).ajaxStop(function(){
            ok( true, "ajaxStop" );
            start();
        }).ajaxSend(function(){
            ok( true, "ajaxSend" );
        }).ajaxComplete(function(){
            ok( true, "ajaxComplete" );
        }).ajaxError(function(){
            ok( false, "ajaxError" );
        }).ajaxSuccess(function(){
            ok( true, "ajaxSuccess" );
        });
        
        $.ajax({
            url: url("data/name.html"),
            beforeSend: function(){ ok(true, "beforeSend"); },
            success: function(){ ok(true, "success"); },
            error: function(){ ok(false, "error"); },
            complete: function(){ ok(true, "complete"); }
        });
    }, 13);
});




































test("$.ajax() - disabled globals", function() {
	expect( 3 );
	stop();
	
	$('#foo').ajaxStart(function(){
		ok( false, "ajaxStart" );
	}).ajaxStop(function(){
		ok( false, "ajaxStop" );
	}).ajaxSend(function(){
		ok( false, "ajaxSend" );
	}).ajaxComplete(function(){
		ok( false, "ajaxComplete" );
	}).ajaxError(function(){
		ok( false, "ajaxError" );
	}).ajaxSuccess(function(){
		ok( false, "ajaxSuccess" );
	});
	
	$.ajax({
		global: false,
		url: url("data/name.html"),
		beforeSend: function(){ ok(true, "beforeSend"); },
		success: function(){ ok(true, "success"); },
		error: function(){ ok(false, "error"); },
		complete: function(){
		  ok(true, "complete");
		  setTimeout(function(){ start(); }, 13);
        }
	});
});

test("$.ajax - xml: non-namespace elements inside namespaced elements", function() {
	expect(3);
	stop();
	$.ajax({
	  url: url("data/with_fries.xml"),
	  dataType: "xml",
	  success: function(resp) {
	    equals( $("properties", resp).length, 1, 'properties in responseXML' );
	    equals( $("jsconf", resp).length, 1, 'jsconf in responseXML' );
	    equals( $("thing", resp).length, 2, 'things in responseXML' );
	    start();
	  }
	});
});

test("$.ajax - beforeSend", function() {
	expect(1);
	stop();
	
	var check = false;
	
	$.ajaxSetup({ timeout: 0 });
	
	$.ajax({
		url: url("data/name.html"), 
		beforeSend: function(xml) {
			check = true;
		},
		success: function(data) {
			ok( check, "check beforeSend was executed" );
			start();
		}
	});
});

test("$.ajax - beforeSend, cancel request (#2688)", function() {
	expect(2);
	var request = $.ajax({
		url: url("data/name.html"), 
		beforeSend: function() {
			ok( true, "beforeSend got called, canceling" );
			return false;
		},
		success: function() {
			ok( false, "request didn't get canceled" );
		},
		complete: function() {
			ok( false, "request didn't get canceled" );
		},
		error: function() {
			ok( false, "request didn't get canceled" );
		}
	});
	ok( request === false, "canceled request must return false instead of XMLHttpRequest instance" );
});

var foobar;

test("$.ajax - dataType html", function() {
	expect(5);
	stop();
	
	foobar = null;
	testFoo = undefined;

	var verifyEvaluation = function() {
	  equals( testFoo, "foo", 'Check if script was evaluated for datatype html' );
	  equals( foobar, "bar", 'Check if script src was evaluated for datatype html' );
	  start();
	};

	$.ajax({
	  dataType: "html",
	  url: url("data/test.html"),
	  success: function(data) {
	  	$("#ap").html(data);
	    ok( data.match(/^html text/), 'Check content for datatype html' );
	    setTimeout(verifyEvaluation, 600);
	  }
	});
});

test("serialize()", function() {
	expect(6);
	
	equals( $('#form').serialize(),
		"action=Test&radio2=on&check=on&hidden=&foo%5Bbar%5D=&name=name&select1=&select2=3&select3=1&select3=2",
		'Check form serialization as query string');
		
	equals( $('#form :input').serialize(),
		"action=Test&radio2=on&check=on&hidden=&foo%5Bbar%5D=&name=name&select1=&select2=3&select3=1&select3=2",
		'Check input serialization as query string');
	
	equals( $('#testForm').serialize(), 
		'T3=%3F%0AZ&H1=x&H2=&PWD=&T1=&T2=YES&My+Name=me&S1=abc&S3=YES&S4=', 
		'Check form serialization as query string');
		
	equals( $('#testForm :input').serialize(), 
		'T3=%3F%0AZ&H1=x&H2=&PWD=&T1=&T2=YES&My+Name=me&S1=abc&S3=YES&S4=', 
		'Check input serialization as query string');
		
	equals( $('#form, #testForm').serialize(),
		"action=Test&radio2=on&check=on&hidden=&foo%5Bbar%5D=&name=name&select1=&select2=3&select3=1&select3=2&T3=%3F%0AZ&H1=x&H2=&PWD=&T1=&T2=YES&My+Name=me&S1=abc&S3=YES&S4=",
		'Multiple form serialization as query string');
		
	equals( $('#form, #testForm :input').serialize(),
		"action=Test&radio2=on&check=on&hidden=&foo%5Bbar%5D=&name=name&select1=&select2=3&select3=1&select3=2&T3=%3F%0AZ&H1=x&H2=&PWD=&T1=&T2=YES&My+Name=me&S1=abc&S3=YES&S4=",
		'Mixed form/input serialization as query string');
});

test("$.param()", function() {
	expect(4);
	var params = {foo:"bar", baz:42, quux:"All your base are belong to us"};
	equals( $.param(params), "foo=bar&baz=42&quux=All+your+base+are+belong+to+us", "simple" );
	
	params = {someName: [1, 2, 3], regularThing: "blah" };
	equals( $.param(params), "someName=1&someName=2&someName=3&regularThing=blah", "with array" );
	
	params = {"foo[]":["baz", 42, "All your base are belong to us"]};
	equals( $.param(params), "foo%5B%5D=baz&foo%5B%5D=42&foo%5B%5D=All+your+base+are+belong+to+us", "more array" );
	
	params = {"foo[bar]":"baz", "foo[beep]":42, "foo[quux]":"All your base are belong to us"};
	equals( $.param(params), "foo%5Bbar%5D=baz&foo%5Bbeep%5D=42&foo%5Bquux%5D=All+your+base+are+belong+to+us", "even more arrays" );
});

test("synchronous request", function() {
	expect(1);
	ok( /^{ "data"/.test( $.ajax({url: url("data/json_obj.js"), async: false}).responseText ), "check returned text" );
});

test("synchronous request with callbacks", function() {
	expect(2);
	var result;
	$.ajax({url: url("data/json_obj.js"), async: false, success: function(data) { ok(true, "success callback executed"); result = data; } });
	ok( /^{ "data"/.test( result ), "check returned text" );
});

test("pass-through request object", function() {
	expect(8);
	stop(true);
	
	var target = "data/name.html";
	var successCount = 0;
	var errorCount = 0;
  var errorEx = "";
	var success = function() {
		successCount++;
	};
	$("#foo").ajaxError(function (e, xml, s, ex) {
		errorCount++;
    errorEx += ": " + xml.status;
	});
	$("#foo").one('ajaxStop', function () {
		equals(successCount, 5, "Check all ajax calls successful");
		equals(errorCount, 0, "Check no ajax errors (status" + errorEx + ")");
		$("#foo").unbind('ajaxError');
		start();
	});
	
	ok( $.get(url(target), success), "get" );
	ok( $.post(url(target), success), "post" );
	ok( $.getScript(url("data/test.js"), success), "script" );
	ok( $.getJSON(url("data/json_obj.js"), success), "json" );
	ok( $.ajax({url: url(target), success: success}), "generic" );
});

































test("global ajaxSettings", function() {
	expect(2);

	var tmp = jQuery.extend({}, jQuery.ajaxSettings);
    var orig = { url: "data/with_fries.xml" };
	var t;

	$.ajaxSetup({ data: {foo: 'bar', bar: 'BAR'} });

    t = jQuery.extend({}, orig);
	t.data = {};
    $.ajax(t);
	ok( t.url.indexOf('foo') > -1 && t.url.indexOf('bar') > -1, "Check extending {}" );

    t = jQuery.extend({}, orig);
	t.data = { zoo: 'a', ping: 'b' };
    $.ajax(t);
	ok( t.url.indexOf('ping') > -1 && t.url.indexOf('zoo') > -1 && t.url.indexOf('foo') > -1 && t.url.indexOf('bar') > -1, "Check extending { zoo: 'a', ping: 'b' }" );
	
	jQuery.ajaxSettings = tmp;
});

test("load(String)", function() {
	expect(1);
	stop(true); 
	$('#first').load("data/name.html", start);
});

test("load('url selector')", function() {
	expect(1);
	stop(true); 
	$('#first').load("data/test3.html div.user", function(){
		equals( $(this).children("div").length, 2, "Verify that specific elements were injected" );
		start();
	});
});

test("load(String, Function) with ajaxSetup on dataType json, see #2046", function() {
	expect(1);
	stop();
	$.ajaxSetup({ dataType: "json" });
	$("#first").ajaxComplete(function (e, xml, s) {
		equals( s.dataType, "html", "Verify the load() dataType was html" );
		$("#first").unbind("ajaxComplete");
		$.ajaxSetup({ dataType: "" });
		start();
	});
	$('#first').load("data/test3.html");
});

test("load(String, Function) - simple: inject text into DOM", function() {
	expect(2);
	stop();
	$('#first').load(url("data/name.html"), function() {
		ok( /^ERROR/.test($('#first').text()), 'Check if content was injected into the DOM' );
		start();
	});
});

test("load(String, Function) - check scripts", function() {
	expect(7);
	stop();
	window.testFoo = undefined;
	window.foobar = null;
	var verifyEvaluation = function() {
		equals( foobar, "bar", 'Check if script src was evaluated after load' );
		equals( $('#ap').html(), 'bar', 'Check if script evaluation has modified DOM');
		 start();
	};
	$('#first').load(url('data/test.html'), function() {
		ok( $('#first').html().match(/^html text/), 'Check content after loading html' );
		equals( $('#foo').html(), 'foo', 'Check if script evaluation has modified DOM');
		equals( testFoo, "foo", 'Check if script was evaluated after load' );
		setTimeout(verifyEvaluation, 600);
	});
});

test("load(String, Function) - check file with only a script tag", function() {
	expect(3);
	stop();
	testFoo = undefined;
	$('#first').load(url('data/test2.html'), function() {
		equals( $('#foo').html(), 'foo', 'Check if script evaluation has modified DOM');
		equals( testFoo, "foo", 'Check if script was evaluated after load' );
		start();
	});
});

test("$.get(String, Hash, Function) - parse xml and use text() on nodes", function() {
	expect(2);
	stop();
	$.get(url('data/dashboard.xml'), function(xml) {
		var content = [];
		$('tab', xml).each(function() {
			content.push($(this).text());
		});
		equals( content[0], 'blabla', 'Check first tab');
		equals( content[1], 'blublu', 'Check second tab');
		start();
	});
});

test("$.getScript(String, Function) - with callback", function() {
	expect(2);
	stop();
	window.foobar = null;
	$.getScript(url("data/test.js"), function() {
		equals( foobar, "bar", 'Check if script was evaluated' );
		setTimeout(start, 100);
	});
});

test("$.getScript(String, Function) - no callback", function() {
	expect(1);
	stop(true);
	$.getScript(url("data/test.js"), start);
});



























































































































































































































































































































































































































}


