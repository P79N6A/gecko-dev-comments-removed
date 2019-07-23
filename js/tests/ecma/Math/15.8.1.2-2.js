





































gTestfile = '15.8.1.2-2.js';













var SECTION = "15.8.1.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LN10";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete( Math.LN10 ); Math.LN10",  
	      2.302585092994046,      
	      eval("delete(Math.LN10); Math.LN10") );

new TestCase( SECTION,
	      "delete( Math.LN10 ); ",            
	      false,                 
	      eval("delete(Math.LN10)") );

test();
