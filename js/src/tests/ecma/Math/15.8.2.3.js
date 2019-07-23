





































gTestfile = '15.8.2.3.js';
















var SECTION = "15.8.2.3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.asin()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.asin()",
	      Number.NaN,
	      Math.asin() );

new TestCase( SECTION,
	      "Math.asin(void 0)",
	      Number.NaN,
	      Math.asin(void 0) );

new TestCase( SECTION,
	      "Math.asin(null)",
	      0,
	      Math.asin(null) );

new TestCase( SECTION,
	      "Math.asin(NaN)",
	      Number.NaN,
	      Math.asin(Number.NaN)   );

new TestCase( SECTION,
	      "Math.asin('string')",
	      Number.NaN,
	      Math.asin("string")     );

new TestCase( SECTION,
	      "Math.asin('0')",
	      0,
	      Math.asin("0") );

new TestCase( SECTION,
	      "Math.asin('1')",
	      Math.PI/2,
	      Math.asin("1") );

new TestCase( SECTION,
	      "Math.asin('-1')",
	      -Math.PI/2,
	      Math.asin("-1") );

new TestCase( SECTION,
	      "Math.asin(Math.SQRT1_2+'')",
	      Math.PI/4,
	      Math.asin(Math.SQRT1_2+'') );

new TestCase( SECTION,
	      "Math.asin(-Math.SQRT1_2+'')",
	      -Math.PI/4,
	      Math.asin(-Math.SQRT1_2+'') );

new TestCase( SECTION,
	      "Math.asin(1.000001)",
	      Number.NaN,
	      Math.asin(1.000001)     );

new TestCase( SECTION,
	      "Math.asin(-1.000001)",
	      Number.NaN,
	      Math.asin(-1.000001)    );

new TestCase( SECTION,
	      "Math.asin(0)",
	      0,
	      Math.asin(0)            );

new TestCase( SECTION,
	      "Math.asin(-0)",
	      -0,
	      Math.asin(-0)           );

new TestCase( SECTION,
	      "Infinity/Math.asin(-0)",
	      -Infinity,
	      Infinity/Math.asin(-0) );

new TestCase( SECTION,
	      "Math.asin(1)",
	      Math.PI/2,
	      Math.asin(1)            );

new TestCase( SECTION,
	      "Math.asin(-1)",
	      -Math.PI/2,
	      Math.asin(-1)            );

new TestCase( SECTION,
	      "Math.asin(Math.SQRT1_2))",
	      Math.PI/4,
	      Math.asin(Math.SQRT1_2) );

new TestCase( SECTION,
	      "Math.asin(-Math.SQRT1_2))",
	      -Math.PI/4,
	      Math.asin(-Math.SQRT1_2));

test();
