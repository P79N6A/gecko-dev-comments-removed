





































gTestfile = '15.7.3.5-3.js';













var SECTION = "15.7.3.5-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.NEGATIVE_INFINITY";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase( SECTION,
	      "Number.NEGATIVE_INFINITY=0; Number.NEGATIVE_INFINITY",
	      -Infinity,
	      eval("Number.NEGATIVE_INFINITY=0; Number.NEGATIVE_INFINITY") );

test();
