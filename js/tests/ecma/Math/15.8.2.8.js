





































gTestfile = '15.8.2.8.js';

















var SECTION = "15.8.2.8";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.exp(x)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.exp.length",
	      1,
	      Math.exp.length );

new TestCase( SECTION,
	      "Math.exp()",
	      Number.NaN,
	      Math.exp() );

new TestCase( SECTION,
	      "Math.exp(null)",
	      1,
	      Math.exp(null) );

new TestCase( SECTION,
	      "Math.exp(void 0)",
	      Number.NaN,
	      Math.exp(void 0) );

new TestCase( SECTION,
	      "Math.exp(1)",
	      Math.E,
	      Math.exp(1) );

new TestCase( SECTION,
	      "Math.exp(true)",
	      Math.E,
	      Math.exp(true) );

new TestCase( SECTION,
	      "Math.exp(false)",
	      1,
	      Math.exp(false) );

new TestCase( SECTION,
	      "Math.exp('1')",
	      Math.E,
	      Math.exp('1') );

new TestCase( SECTION,
	      "Math.exp('0')",
	      1,
	      Math.exp('0') );

new TestCase( SECTION,
	      "Math.exp(NaN)",
	      Number.NaN,
	      Math.exp(Number.NaN) );

new TestCase( SECTION,
	      "Math.exp(0)",
	      1,
	      Math.exp(0)          );

new TestCase( SECTION,
	      "Math.exp(-0)",
	      1,
	      Math.exp(-0)         );

new TestCase( SECTION,
	      "Math.exp(Infinity)",
	      Number.POSITIVE_INFINITY,
	      Math.exp(Number.POSITIVE_INFINITY) );

new TestCase( SECTION,
	      "Math.exp(-Infinity)", 
	      0,
	      Math.exp(Number.NEGATIVE_INFINITY) );

test();
