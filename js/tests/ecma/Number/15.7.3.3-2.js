





































gTestfile = '15.7.3.3-2.js';














var SECTION = "15.7.3.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.MIN_VALUE";

writeHeaderToLog( SECTION + " "+ TITLE );

var MIN_VAL = 5e-324;

new TestCase(  SECTION,
	       "delete( Number.MIN_VALUE )",   
	       false,
	       eval("delete( Number.MIN_VALUE )") );

new TestCase(  SECTION,
	       "delete( Number.MIN_VALUE ); Number.MIN_VALUE",
	       MIN_VAL,
	       eval("delete( Number.MIN_VALUE );Number.MIN_VALUE") );

test();
