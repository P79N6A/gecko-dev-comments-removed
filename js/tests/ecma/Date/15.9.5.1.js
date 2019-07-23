





































gTestfile = '15.9.5.1.js';












var SECTION = "15.9.5.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.constructor";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Date.prototype.constructor == Date",
	      true,
	      Date.prototype.constructor == Date );
test();
