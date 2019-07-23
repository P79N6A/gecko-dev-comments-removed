





































gTestfile = '7.7.3-2.js';























var SECTION = "7.7.3-2";
var VERSION = "ECMA_1";
var TITLE   = "Numeric Literals";
var BUGNUMBER="122884";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "9",
	      9,
	      9 );

new TestCase( SECTION,
	      "09",
	      9,
	      09 );

new TestCase( SECTION,
	      "099",
	      99,
	      099 );


new TestCase( SECTION,
	      "077",
	      63,
	      077 );

test();
