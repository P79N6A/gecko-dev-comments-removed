





































gTestfile = '15.7.3.6-2.js';












var SECTION = "15.7.3.6-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.POSITIVE_INFINITY";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase(SECTION,
	     "delete( Number.POSITIVE_INFINITY )", 
	     false,   
	     eval("delete( Number.POSITIVE_INFINITY )") );

new TestCase(SECTION,
	     "delete( Number.POSITIVE_INFINITY ); Number.POSITIVE_INFINITY",
	     Infinity, 
	     eval("delete( Number.POSITIVE_INFINITY );Number.POSITIVE_INFINITY") );

test();
