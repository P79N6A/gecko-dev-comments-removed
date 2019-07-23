





































gTestfile = '15.8.1.1-1.js';












var SECTION = "15.8.1.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.E";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.E = 0; Math.E",      
	      2.7182818284590452354, 
	      eval("Math.E=0;Math.E") );

test();
