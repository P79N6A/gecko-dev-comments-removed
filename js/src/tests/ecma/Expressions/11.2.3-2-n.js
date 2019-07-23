





































gTestfile = '11.2.3-2-n.js';




































var SECTION = "11.2.3-2-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function Calls";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "3.valueOf()",
	      3,
	      eval("3.valueOf()") );

new TestCase( SECTION,
	      "(3).valueOf()",
	      3,
	      eval("(3).valueOf()") );

test();

