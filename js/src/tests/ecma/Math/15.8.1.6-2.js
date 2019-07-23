





































gTestfile = '15.8.1.6-2.js';













var SECTION = "15.8.1.6-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.PI";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete Math.PI; Math.PI",   
	      3.1415926535897923846, 
	      eval("delete Math.PI; Math.PI") );

new TestCase( SECTION,
	      "delete Math.PI; Math.PI", 
	      false,   
              eval("delete Math.PI") );

test();
