





































gTestfile = '15.8.1.5-2.js';













var SECTION = "15.8.1.5-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LOG10E";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete Math.LOG10E; Math.LOG10E",  
	      0.4342944819032518,    
	      eval("delete Math.LOG10E; Math.LOG10E") );

new TestCase( SECTION,
	      "delete Math.LOG10E",               
	      false,                 
	      eval("delete Math.LOG10E") );

test();
