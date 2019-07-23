





































gTestfile = '15.7.3.3-1.js';














var SECTION = "15.7.3.3-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.MIN_VALUE";

writeHeaderToLog( SECTION + " "+ TITLE );

var MIN_VAL = 5e-324;

new TestCase(  SECTION,
	       "Number.MIN_VALUE",    
	       MIN_VAL,   
	       Number.MIN_VALUE );

test();
