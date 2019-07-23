





































gTestfile = '15.8.1.4-1.js';













var SECTION = "15.8.1.4-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LOG2E";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.L0G2E=0; Math.LOG2E",
	      1.4426950408889634,    
	      eval("Math.LOG2E=0; Math.LOG2E") );

test();
