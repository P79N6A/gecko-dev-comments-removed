





































gTestfile = '15.4.4.1.js';










var SECTION = "15.4.4.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array.prototype.constructor";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,
	      "Array.prototype.constructor == Array",
	      true,  
	      Array.prototype.constructor == Array);

test();
