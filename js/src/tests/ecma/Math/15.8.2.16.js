





































gTestfile = '15.8.2.16.js';










var SECTION = "15.8.2.16";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.sin(x)";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.sin.length",
	      1,
	      Math.sin.length );

new TestCase( SECTION,
	      "Math.sin()",
	      Number.NaN,
	      Math.sin() );

new TestCase( SECTION,
	      "Math.sin(null)",
	      0,
	      Math.sin(null) );

new TestCase( SECTION,
	      "Math.sin(void 0)",
	      Number.NaN,
	      Math.sin(void 0) );

new TestCase( SECTION,
	      "Math.sin(false)",
	      0,
	      Math.sin(false) );

new TestCase( SECTION,
	      "Math.sin('2.356194490192')",
	      0.7071067811865,
	      Math.sin('2.356194490192') );

new TestCase( SECTION,
	      "Math.sin(NaN)",
	      Number.NaN,
	      Math.sin(Number.NaN) );

new TestCase( SECTION,
	      "Math.sin(0)",
	      0,
	      Math.sin(0) );

new TestCase( SECTION,
	      "Math.sin(-0)",
	      -0,
	      Math.sin(-0));

new TestCase( SECTION,
	      "Math.sin(Infinity)",
	      Number.NaN,
	      Math.sin(Number.POSITIVE_INFINITY));

new TestCase( SECTION,
	      "Math.sin(-Infinity)",
	      Number.NaN,
	      Math.sin(Number.NEGATIVE_INFINITY));

new TestCase( SECTION,
	      "Math.sin(0.7853981633974)",
	      0.7071067811865,
	      Math.sin(0.7853981633974));

new TestCase( SECTION,
	      "Math.sin(1.570796326795)",	
	      1,
	      Math.sin(1.570796326795));

new TestCase( SECTION,
	      "Math.sin(2.356194490192)",	
	      0.7071067811865,
	      Math.sin(2.356194490192));

new TestCase( SECTION,
	      "Math.sin(3.14159265359)",
	      0,
	      Math.sin(3.14159265359));

test();
