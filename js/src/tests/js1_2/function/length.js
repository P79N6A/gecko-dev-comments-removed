





































gTestfile = 'length.js';





















var SECTION = "function/length.js";
var VERSION = "ECMA_1";
var TITLE   = "Function.length";
var BUGNUMBER="104204";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var f = new Function( "a","b", "c", "return f.length");

if ( version() <= 120 ) {

  new TestCase( SECTION,
		'var f = new Function( "a","b", "c", "return f.length"); f()',
		0,
		f() );

  new TestCase( SECTION,
		'var f = new Function( "a","b", "c", "return f.length"); f(1,2,3,4,5)',
		5,
		f(1,2,3,4,5) );
} else {

  new TestCase( SECTION,
		'var f = new Function( "a","b", "c", "return f.length"); f()',
		3,
		f() );

  new TestCase( SECTION,
		'var f = new Function( "a","b", "c", "return f.length"); f(1,2,3,4,5)',
		3,
		f(1,2,3,4,5) );


}
test();
