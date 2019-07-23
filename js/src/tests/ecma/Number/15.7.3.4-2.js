





































gTestfile = '15.7.3.4-2.js';














var SECTION = "15.7.3.4-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.NaN";

writeHeaderToLog( SECTION + " "+ TITLE );

new TestCase(SECTION,
	     "delete( Number.NaN ); Number.NaN",      
	     NaN,       
	     eval("delete( Number.NaN );Number.NaN" ));

new TestCase( SECTION,
	      "delete( Number.NaN )",  
	      false, 
	      eval("delete( Number.NaN )") );

test();
