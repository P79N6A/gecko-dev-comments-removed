





































gTestfile = '15.8.1.2-1.js';












var SECTION = "15.8.1.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.LN10";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.LN10=0; Math.LN10",  
	      2.302585092994046,     
	      eval("Math.LN10=0; Math.LN10") );

test();
