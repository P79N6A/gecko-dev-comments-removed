module("event");

test("bind(), with data", function() {
	expect(3);
	var handler = function(event) {
		ok( event.data, "bind() with data, check passed data exists" );
		equals( event.data.foo, "bar", "bind() with data, Check value of passed data" );
	};
	$("#firstp").bind("click", {foo: "bar"}, handler).click().unbind("click", handler);

	ok( !jQuery.data($("#firstp")[0], "events"), "Event handler unbound when using data." );
});

test("bind(), with data, trigger with data", function() {
	expect(4);
	var handler = function(event, data) {
		ok( event.data, "check passed data exists" );
		equals( event.data.foo, "bar", "Check value of passed data" );
		ok( data, "Check trigger data" );
		equals( data.bar, "foo", "Check value of trigger data" );
	};
	$("#firstp").bind("click", {foo: "bar"}, handler).trigger("click", [{bar: "foo"}]).unbind("click", handler);
});

test("bind(), multiple events at once", function() {
	expect(2);
	var clickCounter = 0,
		mouseoverCounter = 0;
	var handler = function(event) {
		if (event.type == "click")
			clickCounter += 1;
		else if (event.type == "mouseover")
			mouseoverCounter += 1;
	};
	$("#firstp").bind("click mouseover", handler).trigger("click").trigger("mouseover");
	equals( clickCounter, 1, "bind() with multiple events at once" );
	equals( mouseoverCounter, 1, "bind() with multiple events at once" );
});

test("bind(), no data", function() {
	expect(1);
	var handler = function(event) {
		ok ( !event.data, "Check that no data is added to the event object" );
	};
	$("#firstp").bind("click", handler).trigger("click");
});

test("bind(), iframes", function() {
	
	
	
	
	
	
	
	
	
	
});

test("bind(), trigger change on select", function() {
	expect(3);
	var counter = 0;
	function selectOnChange(event) {
		equals( event.data, counter++, "Event.data is not a global event object" );
	};
	$("#form select").each(function(i){
		$(this).bind('change', i, selectOnChange);
	}).trigger('change');
});

test("bind(), namespaced events, cloned events", function() {
	expect(6);

	$("#firstp").bind("custom.test",function(e){
		ok(true, "Custom event triggered");
	});

	$("#firstp").bind("click",function(e){
		ok(true, "Normal click triggered");
	});

	$("#firstp").bind("click.test",function(e){
		ok(true, "Namespaced click triggered");
	});

	
	$("#firstp").trigger("click");

	
	$("#firstp").trigger("click.test");

	
	$("#firstp").unbind("click.test");

	
	$("#firstp").trigger("click");

	
	$("#firstp").unbind(".test");

	
	$("#firstp").trigger("custom");

	
	$("#nonnodes").contents().bind("tester", function () {
		equals(this.nodeType, 1, "Check node,textnode,comment bind just does real nodes" );
	}).trigger("tester");

	
	$("<a href='#fail' class='test'>test</a>").click(function(){ return false; }).appendTo("p");
	ok( $("a.test:first").triggerHandler("click") === false, "Handler is bound to appendTo'd elements" );
});

test("trigger() shortcuts", function() {
	expect(6);
	$('<li><a href="#">Change location</a></li>').prependTo('#firstUL').find('a').bind('click', function() {
		var close = $('spanx', this); 
		equals( close.length, 0, "Context element does not exist, length must be zero" );
		ok( !close[0], "Context element does not exist, direct access to element must return undefined" );
		return false;
	}).click();
	
	$("#check1").click(function() {
		ok( true, "click event handler for checkbox gets fired twice, see #815" );
	}).click();
	
	var counter = 0;
	$('#firstp')[0].onclick = function(event) {
		counter++;
	};
	$('#firstp').click();
	equals( counter, 1, "Check that click, triggers onclick event handler also" );
	
	var clickCounter = 0;
	$('#simon1')[0].onclick = function(event) {
		clickCounter++;
	};
	$('#simon1').click();
	equals( clickCounter, 1, "Check that click, triggers onclick event handler on an a tag also" );
	
	$('<img />').load(function(){
		ok( true, "Trigger the load event, using the shortcut .load() (#2819)");
	}).load();
});

test("unbind(event)", function() {
	expect(8);
	var el = $("#firstp");
	el.click(function() {
		ok( true, "Fake normal bind" );
	});
	el.click(function(event) {
		el.unbind(event);
		ok( true, "Fake onebind" );
	});
	el.click().click();
	
	el.click(function() { return; });
	el.unbind('click');
	ok( !el[0].onclick, "Handler is removed" ); 

	el.click(function() { return; });
	el.unbind('change',function(){ return; });
	for (var ret in jQuery.data(el[0], "events")['click']) break;
	ok( ret, "Extra handlers weren't accidentally removed." );

	el.unbind('click');
	ok( !jQuery.data(el[0], "events"), "Removed the events expando after all handlers are unbound." );
	
	reset();
	var clickCounter = (mouseoverCounter = 0);
	var handler = function(event) {
		if (event.type == "click")
			clickCounter += 1;
		else if (event.type == "mouseover")
			mouseoverCounter += 1;
	};
	$("#firstp").bind("click mouseover", handler).unbind("click mouseover", handler).trigger("click").trigger("mouseover");
	equals( clickCounter, 0, "unbind() with multiple events at once" );
	equals( mouseoverCounter, 0, "unbind() with multiple events at once" );
});

test("trigger(event, [data], [fn])", function() {
	expect(67);

	var handler = function(event, a, b, c) {
		equals( event.type, "click", "check passed data" );
		equals( a, 1, "check passed data" );
		equals( b, "2", "check passed data" );
		equals( c, "abc", "check passed data" );
		return "test";
	};

	var handler2 = function(a, b, c) {
		equals( a, 1, "check passed data" );
		equals( b, "2", "check passed data" );
		equals( c, "abc", "check passed data" );
		return false;
	};

	var handler3 = function(a, b, c, v) {
		equals( a, 1, "check passed data" );
		equals( b, "2", "check passed data" );
		equals( c, "abc", "check passed data" );
		equals( v, "test", "check current value" );
		return "newVal";
	};

	var handler4 = function(a, b, c, v) {
		equals( a, 1, "check passed data" );
		equals( b, "2", "check passed data" );
		equals( c, "abc", "check passed data" );
		equals( v, "test", "check current value" );
	};

	
	$("#firstp")[0].click = function(){
		ok( true, "Native call was triggered" );
	};

	
	
	$("#firstp").bind("click", handler).trigger("click", [1, "2", "abc"]);

	
	
	$("#firstp").trigger("click", [1, "2", "abc"], handler4);

	
	$("#firstp")[0].click = function(){
		ok( false, "Native call was triggered" );
	};

	
	
	$("#firstp").trigger("click", [1, "2", "abc"], handler2);

	
	
	equals( $("#firstp").triggerHandler("click", [1, "2", "abc"]), "test", "Verify handler response" );

	
	
	equals( $("#firstp").triggerHandler("click", [1, "2", "abc"], handler2), false, "Verify handler response" );

	
	var eventObj = jQuery.event.fix({ type: "foo", target: document.body });

	
	
	equals( $("#firstp").triggerHandler("click", [eventObj, 1, "2", "abc"]), "test", "Verify handler response" );

	
	
	eventObj = jQuery.event.fix({ type: "foo", target: document.body });
	equals( $("#firstp").triggerHandler("click", [eventObj, 1, "2", "abc"], handler), "test", "Verify handler response" );
	
	var pass = true;
	try {
		$('input:first')
			.hide()
			.trigger('focus');
	} catch(e) {
		pass = false;
	}
	ok( pass, "Trigger focus on hidden element" );

	
	
	equals( $("#firstp").triggerHandler("click", [1, "2", "abc"], handler3), "newVal", "Verify triggerHandler return is overwritten by extra function" );

	
	
	equals( $("#firstp").triggerHandler("click", [1, "2", "abc"], handler4), "test", "Verify triggerHandler return is not overwritten by extra function" );
});

test("toggle(Function, Function, ...)", function() {
	expect(11);
	
	var count = 0,
		fn1 = function(e) { count++; },
		fn2 = function(e) { count--; },
		preventDefault = function(e) { e.preventDefault() },
		link = $('#mark');
	link.click(preventDefault).click().toggle(fn1, fn2).click().click().click().click().click();
	equals( count, 1, "Check for toggle(fn, fn)" );

	$("#firstp").toggle(function () {
		equals(arguments.length, 4, "toggle correctly passes through additional triggered arguments, see #1701" )
	}, function() {}).trigger("click", [ 1, 2, 3 ]);

	var first = 0;
	$("#simon1").one("click", function() {
		ok( true, "Execute event only once" );
		$(this).toggle(function() {
			equals( first++, 0, "toggle(Function,Function) assigned from within one('xxx'), see #1054" );
		}, function() {
			equals( first, 1, "toggle(Function,Function) assigned from within one('xxx'), see #1054" );
		});
		return false;
	}).click().click().click();
	
	var turn = 0;
	var fns = [
		function(){
			turn = 1;
		},
		function(){
			turn = 2;
		},
		function(){
			turn = 3;
		}
	];
	
	var $div = $("<div>&nbsp;</div>").toggle( fns[0], fns[1], fns[2] );
	$div.click();
	equals( turn, 1, "Trying toggle with 3 functions, attempt 1 yields 1");
	$div.click();
	equals( turn, 2, "Trying toggle with 3 functions, attempt 2 yields 2");
	$div.click();
	equals( turn, 3, "Trying toggle with 3 functions, attempt 3 yields 3");
	$div.click();
	equals( turn, 1, "Trying toggle with 3 functions, attempt 4 yields 1");
	$div.click();
	equals( turn, 2, "Trying toggle with 3 functions, attempt 5 yields 2");
	
	$div.unbind('click',fns[0]);
	var data = $.data( $div[0], 'events' );
	ok( !data, "Unbinding one function from toggle unbinds them all");
});

test("jQuery(function($) {})", function() {
	stop();
	jQuery(function($) {
		equals(jQuery, $, "ready doesn't provide an event object, instead it provides a reference to the jQuery function, see http://docs.jquery.com/Events/ready#fn");
		start();
	});
});

test("event properties", function() {
	stop();
	$("#simon1").click(function(event) {
		ok( event.timeStamp, "assert event.timeStamp is present" );
		start();
	}).click();
});
