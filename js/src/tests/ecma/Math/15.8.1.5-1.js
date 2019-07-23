





































gTestfile = '15.8.1.5-1.js';














var SECTION = "15.8.1.5-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LOG10E";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.LOG10E=0; Math.LOG10E",
	      0.4342944819032518,  
	      eval("Math.LOG10E=0; Math.LOG10E") );

test();
