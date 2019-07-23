





































gTestfile = '15.8.2.18.js';















var SECTION = "15.8.2.18";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.tan(x)";
var EXCLUDE = "true";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.tan.length",
	      1,
	      Math.tan.length );

new TestCase( SECTION,
	      "Math.tan()",
	      Number.NaN,
	      Math.tan() );

new TestCase( SECTION,
	      "Math.tan(void 0)",
	      Number.NaN,
	      Math.tan(void 0));

new TestCase( SECTION,
	      "Math.tan(null)",
	      0,
	      Math.tan(null) );

new TestCase( SECTION,
	      "Math.tan(false)",
	      0,
	      Math.tan(false) );

new TestCase( SECTION,
	      "Math.tan(NaN)",
	      Number.NaN,
	      Math.tan(Number.NaN) );

new TestCase( SECTION,
	      "Math.tan(0)",
	      0,
	      Math.tan(0));

new TestCase( SECTION,
	      "Math.tan(-0)",
	      -0,
	      Math.tan(-0));

new TestCase( SECTION,
	      "Math.tan(Infinity)",
	      Number.NaN,
	      Math.tan(Number.POSITIVE_INFINITY));

new TestCase( SECTION,
	      "Math.tan(-Infinity)",
	      Number.NaN,
	      Math.tan(Number.NEGATIVE_INFINITY));

new TestCase( SECTION,
	      "Math.tan(Math.PI/4)",
	      1,
	      Math.tan(Math.PI/4));

new TestCase( SECTION,
	      "Math.tan(3*Math.PI/4)",
	      -1,
	      Math.tan(3*Math.PI/4));

new TestCase( SECTION,
	      "Math.tan(Math.PI)",
	      -0,
	      Math.tan(Math.PI));

new TestCase( SECTION,
	      "Math.tan(5*Math.PI/4)",
	      1,
	      Math.tan(5*Math.PI/4));

new TestCase( SECTION,
	      "Math.tan(7*Math.PI/4)",
	      -1,
	      Math.tan(7*Math.PI/4));

new TestCase( SECTION,
	      "Infinity/Math.tan(-0)",
	      -Infinity,
	      Infinity/Math.tan(-0) );













new TestCase( SECTION,
	      "Math.tan(3*Math.PI/2) >= 5443000000000000",
	      true,
	      Math.tan(3*Math.PI/2) >= 5443000000000000 );

new TestCase( SECTION,
	      "Math.tan(Math.PI/2) >= 5443000000000000",
	      true,
	      Math.tan(Math.PI/2) >= 5443000000000000 );

test();
