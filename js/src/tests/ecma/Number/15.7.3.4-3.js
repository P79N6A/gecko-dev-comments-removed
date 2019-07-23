





































gTestfile = '15.7.3.4-3.js';













var SECTION = "15.7.3.4-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.NaN";

writeHeaderToLog( SECTION + " "+ TITLE );

new TestCase( SECTION,
	      "Number.NaN=0; Number.NaN",
	      Number.NaN,
	      eval("Number.NaN=0; Number.NaN") );

test();
