





































gTestfile = '15.8.1.7-1.js';













var SECTION = "15.8.1.7-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.SQRT1_2";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.SQRT1_2=0; Math.SQRT1_2",
	      0.7071067811865476,
	      eval("Math.SQRT1_2=0; Math.SQRT1_2") );

test();
