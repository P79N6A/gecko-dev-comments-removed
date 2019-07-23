





































gTestfile = '15.5.3.2-2.js';




















var SECTION = "15.5.3.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.fromCharCode()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var MYSTRING = String.fromCharCode(eval(\"var args=''; for ( i = 0x0020; i < 0x007f; i++ ) { args += ( i == 0x007e ) ? i : i + ', '; } args;\")); MYSTRING",
	      " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~",
	      eval( "var MYSTRING = String.fromCharCode(" + eval("var args=''; for ( i = 0x0020; i < 0x007f; i++ ) { args += ( i == 0x007e ) ? i : i + ', '; } args;") +"); MYSTRING" ));

new TestCase( SECTION,
	      "MYSTRING.length",
	      0x007f - 0x0020,
	      MYSTRING.length );

test();
