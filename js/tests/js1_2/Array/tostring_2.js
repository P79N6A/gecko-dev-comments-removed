





































gTestfile = 'tostring_2.js';











var SECTION = "Array/tostring_2.js";
var VERSION = "JS_12";
startTest();
var TITLE   = "Array.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

var a = [];


if ( version() == 120 ) {
  VERSION = "120";
} else {
  VERSION = "";
}

new TestCase ( SECTION,
	       "a.toString()",
	       ( VERSION == "120" ? "[]" : "" ),
	       a.toString() );

new TestCase ( SECTION,
	       "String( a )",
	       ( VERSION == "120" ? "[]" : "" ),
	       String( a ) );

new TestCase ( SECTION,
	       "a +''",
	       ( VERSION == "120" ? "[]" : "" ),
	       a+"" );

test();
