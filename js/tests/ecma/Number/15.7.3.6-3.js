





































gTestfile = '15.7.3.6-3.js';













var SECTION = "15.7.3.6-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.POSITIVE_INFINITY";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase( SECTION,
	      "Number.POSITIVE_INFINITY=0; Number.POSITIVE_INFINITY",
	      Number.POSITIVE_INFINITY,
	      eval("Number.POSITIVE_INFINITY=0; Number.POSITIVE_INFINITY") );

test();
