





































gTestfile = '15.8.1.6-1.js';













var SECTION = "15.8.1.6-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.PI";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.PI=0; Math.PI",      
	      3.1415926535897923846, 
	      eval("Math.PI=0; Math.PI") );

test();
