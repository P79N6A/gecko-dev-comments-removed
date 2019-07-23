





































gTestfile = '15.3.5.1.js';





















var SECTION = "15.3.5.1";
var VERSION = "ECMA_1";
var TITLE   = "Function.length";
var BUGNUMBER="104204";
startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

var f = new Function( "a","b", "c", "return f.length");

new TestCase( SECTION,
	      'var f = new Function( "a","b", "c", "return f.length"); f()',
	      3,
	      f() );


new TestCase( SECTION,
	      'var f = new Function( "a","b", "c", "return f.length"); f(1,2,3,4,5)',
	      3,
	      f(1,2,3,4,5) );

test();

