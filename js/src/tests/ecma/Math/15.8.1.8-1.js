





































gTestfile = '15.8.1.8-1.js';













var SECTION = "15.8.1.8-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.SQRT2";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.SQRT2=0; Math.SQRT2",
	      1.4142135623730951,    
	      eval("Math.SQRT2=0; Math.SQRT2") );

test();
