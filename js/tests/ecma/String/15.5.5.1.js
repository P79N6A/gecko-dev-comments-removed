





































gTestfile = '15.5.5.1.js';
















var SECTION = "15.5.5.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.length";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"var s = new String(); s.length",
		0,
		eval("var s = new String(); s.length") );

new TestCase(   SECTION,
		"var s = new String(); s.length = 10; s.length",
		0,
		eval("var s = new String(); s.length = 10; s.length") );

new TestCase(   SECTION,
		"var s = new String(); var props = ''; for ( var p in s ) {  props += p; };  props",
		"",
		eval("var s = new String(); var props = ''; for ( var p in s ) {  props += p; };  props") );

new TestCase(   SECTION,
		"var s = new String(); delete s.length",
		false,
		eval("var s = new String(); delete s.length") );

new TestCase(   SECTION,
		"var s = new String('hello'); delete s.length; s.length",
		5,
		eval("var s = new String('hello'); delete s.length; s.length") );

test();
