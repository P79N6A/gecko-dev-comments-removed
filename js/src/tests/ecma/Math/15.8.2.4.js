





































gTestfile = '15.8.2.4.js';

















var SECTION = "15.8.2.4";
var VERSION = "ECMA_1";
var TITLE   = "Math.atan()";
var BUGNUMBER="77391";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.atan.length",
	      1,
	      Math.atan.length );

new TestCase( SECTION,
	      "Math.atan()",
	      Number.NaN,
	      Math.atan() );

new TestCase( SECTION,
	      "Math.atan(void 0)",
	      Number.NaN,
	      Math.atan(void 0) );

new TestCase( SECTION,
	      "Math.atan(null)",
	      0,
	      Math.atan(null) );

new TestCase( SECTION,
	      "Math.atan(NaN)",
	      Number.NaN,
	      Math.atan(Number.NaN) );

new TestCase( SECTION,
	      "Math.atan('a string')",
	      Number.NaN,
	      Math.atan("a string") );

new TestCase( SECTION,
	      "Math.atan('0')",
	      0,
	      Math.atan('0') );

new TestCase( SECTION,
	      "Math.atan('1')",
	      Math.PI/4,
	      Math.atan('1') );

new TestCase( SECTION,
	      "Math.atan('-1')",
	      -Math.PI/4,
	      Math.atan('-1') );

new TestCase( SECTION,
	      "Math.atan('Infinity)",
	      Math.PI/2,
	      Math.atan('Infinity') );

new TestCase( SECTION,
	      "Math.atan('-Infinity)",
	      -Math.PI/2,
	      Math.atan('-Infinity') );

new TestCase( SECTION,
	      "Math.atan(0)",
	      0,
	      Math.atan(0)          );

new TestCase( SECTION,
	      "Math.atan(-0)",	
	      -0,
	      Math.atan(-0)         );

new TestCase( SECTION,
	      "Infinity/Math.atan(-0)",
	      -Infinity,
	      Infinity/Math.atan(-0) );

new TestCase( SECTION,
	      "Math.atan(Infinity)",
	      Math.PI/2,
	      Math.atan(Number.POSITIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.atan(-Infinity)",
	      -Math.PI/2,
	      Math.atan(Number.NEGATIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.atan(1)",
	      Math.PI/4,
	      Math.atan(1)          );

new TestCase( SECTION,
	      "Math.atan(-1)",
	      -Math.PI/4,
	      Math.atan(-1)         );

test();
