





































gTestfile = '15.8.2.6.js';



















var SECTION = "15.8.2.6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.ceil(x)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.ceil.length",
	      1,
	      Math.ceil.length );

new TestCase( SECTION,
	      "Math.ceil(NaN)",
	      Number.NaN,
	      Math.ceil(Number.NaN)   );

new TestCase( SECTION,
	      "Math.ceil(null)",
	      0, 
	      Math.ceil(null) );

new TestCase( SECTION,
	      "Math.ceil()",
	      Number.NaN,
	      Math.ceil() );

new TestCase( SECTION,
	      "Math.ceil(void 0)",
	      Number.NaN,
	      Math.ceil(void 0) );

new TestCase( SECTION,
	      "Math.ceil('0')",
	      0,
	      Math.ceil('0')            );

new TestCase( SECTION,
	      "Math.ceil('-0')",
	      -0,
	      Math.ceil('-0')           );

new TestCase( SECTION,
	      "Infinity/Math.ceil('0')",
	      Infinity,
	      Infinity/Math.ceil('0'));

new TestCase( SECTION,
	      "Infinity/Math.ceil('-0')",
	      -Infinity,
	      Infinity/Math.ceil('-0'));

new TestCase( SECTION,
	      "Math.ceil(0)",
	      0,
	      Math.ceil(0)            );

new TestCase( SECTION,
	      "Math.ceil(-0)",
	      -0,
	      Math.ceil(-0)           );

new TestCase( SECTION,
	      "Infinity/Math.ceil(0)",
	      Infinity,
	      Infinity/Math.ceil(0));

new TestCase( SECTION,
	      "Infinity/Math.ceil(-0)",
	      -Infinity,
	      Infinity/Math.ceil(-0));


new TestCase( SECTION,
	      "Math.ceil(Infinity)",
	      Number.POSITIVE_INFINITY,
	      Math.ceil(Number.POSITIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.ceil(-Infinity)",
	      Number.NEGATIVE_INFINITY,
	      Math.ceil(Number.NEGATIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.ceil(-Number.MIN_VALUE)",
	      -0,
	      Math.ceil(-Number.MIN_VALUE) );

new TestCase( SECTION,
	      "Infinity/Math.ceil(-Number.MIN_VALUE)",
	      -Infinity,
	      Infinity/Math.ceil(-Number.MIN_VALUE) );

new TestCase( SECTION,
	      "Math.ceil(1)",
	      1,
	      Math.ceil(1)   );

new TestCase( SECTION,
	      "Math.ceil(-1)",
	      -1,
	      Math.ceil(-1)   );

new TestCase( SECTION,
	      "Math.ceil(-0.9)",
	      -0,
	      Math.ceil(-0.9) );

new TestCase( SECTION,
	      "Infinity/Math.ceil(-0.9)",
	      -Infinity,
	      Infinity/Math.ceil(-0.9) );

new TestCase( SECTION,
	      "Math.ceil(0.9 )",
	      1,
	      Math.ceil( 0.9) );

new TestCase( SECTION,
	      "Math.ceil(-1.1)",
	      -1,
	      Math.ceil( -1.1));

new TestCase( SECTION,
	      "Math.ceil( 1.1)",
	      2,
	      Math.ceil(  1.1));

new TestCase( SECTION,
	      "Math.ceil(Infinity)",
	      -Math.floor(-Infinity),
	      Math.ceil(Number.POSITIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.ceil(-Infinity)",
	      -Math.floor(Infinity),
	      Math.ceil(Number.NEGATIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.ceil(-Number.MIN_VALUE)",
	      -Math.floor(Number.MIN_VALUE),
	      Math.ceil(-Number.MIN_VALUE) );

new TestCase( SECTION,
	      "Math.ceil(1)",
	      -Math.floor(-1),
	      Math.ceil(1)   );

new TestCase( SECTION,
	      "Math.ceil(-1)",
	      -Math.floor(1),
	      Math.ceil(-1)   );

new TestCase( SECTION,
	      "Math.ceil(-0.9)",
	      -Math.floor(0.9),
	      Math.ceil(-0.9) );

new TestCase( SECTION,
	      "Math.ceil(0.9 )",
	      -Math.floor(-0.9),
	      Math.ceil( 0.9) );

new TestCase( SECTION,
	      "Math.ceil(-1.1)",
	      -Math.floor(1.1),
	      Math.ceil( -1.1));

new TestCase( SECTION,
	      "Math.ceil( 1.1)",
	      -Math.floor(-1.1),
	      Math.ceil(  1.1));

test();
