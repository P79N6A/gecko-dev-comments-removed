





































gTestfile = '15.8.1.8-2.js';












var SECTION = "15.8.1.8-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.SQRT2";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete Math.SQRT2; Math.SQRT2",
	      1.4142135623730951,    
	      eval("delete Math.SQRT2; Math.SQRT2") );

new TestCase( SECTION,
	      "delete Math.SQRT2",            
	      false,                 
	      eval("delete Math.SQRT2") );

test();
