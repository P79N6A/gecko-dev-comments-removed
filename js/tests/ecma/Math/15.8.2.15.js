





































gTestfile = '15.8.2.15.js';































var SECTION = "15.8.2.15";
var VERSION = "ECMA_1";
var TITLE   = "Math.round(x)";
var BUGNUMBER="331411";

var EXCLUDE = "true";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.round.length",
	      1,
	      Math.round.length );

new TestCase( SECTION,
	      "Math.round()",
	      Number.NaN,
	      Math.round() );

new TestCase( SECTION,
	      "Math.round(null)",
	      0,
	      Math.round(0) );

new TestCase( SECTION,
	      "Math.round(void 0)",
	      Number.NaN,
	      Math.round(void 0) );

new TestCase( SECTION,
	      "Math.round(true)",
	      1,
	      Math.round(true) );

new TestCase( SECTION,
	      "Math.round(false)",
	      0,
	      Math.round(false) );

new TestCase( SECTION,
	      "Math.round('.99999')",
	      1,
	      Math.round('.99999') );

new TestCase( SECTION,
	      "Math.round('12345e-2')",
	      123,
	      Math.round('12345e-2') );

new TestCase( SECTION,
	      "Math.round(NaN)",
	      Number.NaN,
	      Math.round(Number.NaN) );

new TestCase( SECTION,
	      "Math.round(0)",
	      0,
	      Math.round(0) );

new TestCase( SECTION,
	      "Math.round(-0)",
	      -0,
	      Math.round(-0));

new TestCase( SECTION,
	      "Infinity/Math.round(-0)",
	      -Infinity,
	      Infinity/Math.round(-0) );

new TestCase( SECTION,
	      "Math.round(Infinity)",
	      Number.POSITIVE_INFINITY,
	      Math.round(Number.POSITIVE_INFINITY));

new TestCase( SECTION,
	      "Math.round(-Infinity)",
	      Number.NEGATIVE_INFINITY,
	      Math.round(Number.NEGATIVE_INFINITY));

new TestCase( SECTION,
	      "Math.round(0.49)",
	      0,
	      Math.round(0.49));

new TestCase( SECTION,
	      "Math.round(0.5)",
	      1,
	      Math.round(0.5));

new TestCase( SECTION,
	      "Math.round(0.51)",
	      1,
	      Math.round(0.51));

new TestCase( SECTION,
	      "Math.round(-0.49)",
	      -0,
	      Math.round(-0.49));

new TestCase( SECTION,
	      "Math.round(-0.5)",
	      -0,
	      Math.round(-0.5));

new TestCase( SECTION,
	      "Infinity/Math.round(-0.49)",
	      -Infinity,
	      Infinity/Math.round(-0.49));

new TestCase( SECTION,
	      "Infinity/Math.round(-0.5)",
	      -Infinity,
	      Infinity/Math.round(-0.5));

new TestCase( SECTION,
	      "Math.round(-0.51)",
	      -1,
	      Math.round(-0.51));

new TestCase( SECTION,
	      "Math.round(3.5)",
	      4,
	      Math.round(3.5));

new TestCase( SECTION,
	      "Math.round(-3.5)",
	      -3,
	      Math.round(-3));

test();
