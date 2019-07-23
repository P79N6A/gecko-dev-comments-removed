





































gTestfile = '15.6.3.1-3.js';

















var SECTION = "15.6.3.1-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype"
  writeHeaderToLog( SECTION + TITLE );

var array = new Array();
var item = 0;

new TestCase( SECTION,
	      "delete( Boolean.prototype); Boolean.prototype",
	      Boolean.prototype,
	      eval("delete( Boolean.prototype); Boolean.prototype") );

test();
