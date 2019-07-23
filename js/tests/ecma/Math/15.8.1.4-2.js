





































gTestfile = '15.8.1.4-2.js';













var SECTION = "15.8.1.4-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LOG2E";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete(Math.L0G2E);Math.LOG2E",
	      1.4426950408889634,    
	      eval("delete(Math.LOG2E);Math.LOG2E") );
new TestCase( SECTION,
	      "delete(Math.L0G2E)",           
	      false,                 
	      eval("delete(Math.LOG2E)") );

test();
