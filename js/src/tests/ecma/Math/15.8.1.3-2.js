





































gTestfile = '15.8.1.3-2.js';













var SECTION = "15.8.1.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LN2";

writeHeaderToLog( SECTION + " "+ TITLE);

var MATH_LN2 = 0.6931471805599453;

new TestCase( SECTION,
	      "delete(Math.LN2)",             
	      false,         
	      eval("delete(Math.LN2)") );

new TestCase( SECTION,
	      "delete(Math.LN2); Math.LN2",   
	      MATH_LN2,      
	      eval("delete(Math.LN2); Math.LN2") );

test();
