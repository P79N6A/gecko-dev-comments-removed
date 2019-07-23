





































gTestfile = '15.6.4.1.js';












var SECTION = "15.6.4.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype.constructor"
  writeHeaderToLog( SECTION + TITLE );

new TestCase( SECTION,
	      "( Boolean.prototype.constructor == Boolean )",
	      true ,
	      (Boolean.prototype.constructor == Boolean) );
test();
