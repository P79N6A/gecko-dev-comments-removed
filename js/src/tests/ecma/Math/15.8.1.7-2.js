





































gTestfile = '15.8.1.7-2.js';













var SECTION = "15.8.1.7-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.SQRT1_2";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "delete Math.SQRT1_2; Math.SQRT1_2",
	      0.7071067811865476,
	      eval("delete Math.SQRT1_2; Math.SQRT1_2") );

new TestCase( SECTION,
	      "delete Math.SQRT1_2",               
	      false,             
	      eval("delete Math.SQRT1_2") );

test();
