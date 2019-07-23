





































gTestfile = '15.8.1.8-3.js';












var SECTION = "15.8.1.8-3";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Math.SQRT2:  DontDelete");

new TestCase( SECTION,
	      "delete Math.SQRT2",
	      false,    
	      eval("delete Math.SQRT2") );

test();
