





































gTestfile = '15.8.1.1-2.js';












var SECTION = "15.8.1.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.E";

writeHeaderToLog( SECTION + " "+ TITLE);

var MATH_E = 2.7182818284590452354
  new TestCase( SECTION,
		"delete(Math.E)",               
		false,   
		eval("delete Math.E") );
new TestCase( SECTION,
	      "delete(Math.E); Math.E",       
	      MATH_E,  
	      eval("delete Math.E; Math.E") );

test();
