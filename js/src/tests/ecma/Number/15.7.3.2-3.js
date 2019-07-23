





































gTestfile = '15.7.3.2-3.js';













var SECTION = "15.7.3.2-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.MAX_VALUE";

writeHeaderToLog( SECTION + " "+ TITLE );

var MAX_VAL = 1.7976931348623157e308;

new TestCase( SECTION,
	      "Number.MAX_VALUE=0; Number.MAX_VALUE",
	      MAX_VAL,
	      eval("Number.MAX_VALUE=0; Number.MAX_VALUE") );

test();
