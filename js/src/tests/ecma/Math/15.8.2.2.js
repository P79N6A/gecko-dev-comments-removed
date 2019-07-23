





































gTestfile = '15.8.2.2.js';














var SECTION = "15.8.2.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.acos()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.acos.length",
	      1,
	      Math.acos.length );

new TestCase( SECTION,
	      "Math.acos(void 0)",
	      Number.NaN,
	      Math.acos(void 0) );

new TestCase( SECTION,
	      "Math.acos()",
	      Number.NaN,
	      Math.acos() );

new TestCase( SECTION,
	      "Math.acos(null)",
	      Math.PI/2,
	      Math.acos(null) );

new TestCase( SECTION,
	      "Math.acos(NaN)",
	      Number.NaN,
	      Math.acos(Number.NaN) );

new TestCase( SECTION,
	      "Math.acos(a string)",
	      Number.NaN,
	      Math.acos("a string") );

new TestCase( SECTION,
	      "Math.acos('0')",
	      Math.PI/2,
	      Math.acos('0') );

new TestCase( SECTION,
	      "Math.acos('1')",
	      0,
	      Math.acos('1') );

new TestCase( SECTION,
	      "Math.acos('-1')",
	      Math.PI,
	      Math.acos('-1') );

new TestCase( SECTION,
	      "Math.acos(1.00000001)",
	      Number.NaN,
	      Math.acos(1.00000001) );

new TestCase( SECTION,
	      "Math.acos(11.00000001)",
	      Number.NaN,
	      Math.acos(-1.00000001) );

new TestCase( SECTION,
	      "Math.acos(1)",
	      0,
	      Math.acos(1)          );

new TestCase( SECTION,
	      "Math.acos(-1)",
	      Math.PI,
	      Math.acos(-1)         );

new TestCase( SECTION,
	      "Math.acos(0)",
	      Math.PI/2,
	      Math.acos(0)          );

new TestCase( SECTION,
	      "Math.acos(-0)",
	      Math.PI/2,
	      Math.acos(-0)         );

new TestCase( SECTION,
	      "Math.acos(Math.SQRT1_2)",
	      Math.PI/4,
	      Math.acos(Math.SQRT1_2));

new TestCase( SECTION,
	      "Math.acos(-Math.SQRT1_2)",
	      Math.PI/4*3,
	      Math.acos(-Math.SQRT1_2));

new TestCase( SECTION,
	      "Math.acos(0.9999619230642)",
	      Math.PI/360,
	      Math.acos(0.9999619230642));

new TestCase( SECTION,
	      "Math.acos(-3.0)",
	      Number.NaN,
	      Math.acos(-3.0));

test();
