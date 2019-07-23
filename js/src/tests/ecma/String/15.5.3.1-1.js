





































gTestfile = '15.5.3.1-1.js';

















var SECTION = "15.5.3.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the String Constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "String.prototype.length",   0,  String.prototype.length );

new TestCase(   SECTION,
		"var str='';for ( p in String ) { if ( p == 'prototype' ) str += p; } str",
		"",
		eval("var str='';for ( p in String ) { if ( p == 'prototype' ) str += p; } str") );

test();
