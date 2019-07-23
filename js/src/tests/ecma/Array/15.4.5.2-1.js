





































gTestfile = '15.4.5.2-1.js';














var SECTION = "15.4.5.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array.length";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"var A = new Array(); A.length",
		0,
		eval("var A = new Array(); A.length") );
new TestCase(   SECTION,
		"var A = new Array(); A[Math.pow(2,32)-2] = 'hi'; A.length",
		Math.pow(2,32)-1,
		eval("var A = new Array(); A[Math.pow(2,32)-2] = 'hi'; A.length") );
new TestCase(   SECTION,
		"var A = new Array(); A.length = 123; A.length",
		123,
		eval("var A = new Array(); A.length = 123; A.length") );
new TestCase(   SECTION,
		"var A = new Array(); A.length = 123; var PROPS = ''; for ( var p in A ) { PROPS += ( p == 'length' ? p : ''); } PROPS",
		"",
		eval("var A = new Array(); A.length = 123; var PROPS = ''; for ( var p in A ) { PROPS += ( p == 'length' ? p : ''); } PROPS") );
new TestCase(   SECTION,
		"var A = new Array(); A.length = 123; delete A.length",
		false ,
		eval("var A = new Array(); A.length = 123; delete A.length") );
new TestCase(   SECTION,
		"var A = new Array(); A.length = 123; delete A.length; A.length",
		123,
		eval("var A = new Array(); A.length = 123; delete A.length; A.length") );
test();

