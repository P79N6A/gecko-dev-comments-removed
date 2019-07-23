





































gTestfile = '15.8.2.12.js';


















var SECTION = "15.8.2.12";
var VERSION = "ECMA_1";
var TITLE   = "Math.min(x, y)";
var BUGNUMBER="76439";

startTest();

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Math.min.length",
	      2,
	      Math.min.length );

new TestCase( SECTION,
	      "Math.min()",
	      Infinity,
	      Math.min() );

new TestCase( SECTION,
	      "Math.min(void 0, 1)",
	      Number.NaN,
	      Math.min( void 0, 1 ) );

new TestCase( SECTION,
	      "Math.min(void 0, void 0)",
	      Number.NaN,
	      Math.min( void 0, void 0 ) );

new TestCase( SECTION,
	      "Math.min(null, 1)",
	      0,
	      Math.min( null, 1 ) );

new TestCase( SECTION,
	      "Math.min(-1, null)",
	      -1,
	      Math.min( -1, null ) );

new TestCase( SECTION,
	      "Math.min(true, false)",
	      0,
	      Math.min(true,false) );

new TestCase( SECTION,
	      "Math.min('-99','99')",
	      -99,
	      Math.min( "-99","99") );

new TestCase( SECTION,
	      "Math.min(NaN,0)",
	      Number.NaN,
	      Math.min(Number.NaN,0) );

new TestCase( SECTION,
	      "Math.min(NaN,1)",
	      Number.NaN,
	      Math.min(Number.NaN,1) );

new TestCase( SECTION,
	      "Math.min(NaN,-1)",
	      Number.NaN,
	      Math.min(Number.NaN,-1) );

new TestCase( SECTION,
	      "Math.min(0,NaN)",
	      Number.NaN,
	      Math.min(0,Number.NaN) );

new TestCase( SECTION,
	      "Math.min(1,NaN)",
	      Number.NaN,
	      Math.min(1,Number.NaN) );

new TestCase( SECTION,
	      "Math.min(-1,NaN)",
	      Number.NaN,
	      Math.min(-1,Number.NaN) );

new TestCase( SECTION,
	      "Math.min(NaN,NaN)",
	      Number.NaN,
	      Math.min(Number.NaN,Number.NaN) );

new TestCase( SECTION,
	      "Math.min(1,1.0000000001)",
	      1,
	      Math.min(1,1.0000000001) );

new TestCase( SECTION,
	      "Math.min(1.0000000001,1)",
	      1,
	      Math.min(1.0000000001,1) );

new TestCase( SECTION,
	      "Math.min(0,0)",
	      0,
	      Math.min(0,0) );

new TestCase( SECTION,
	      "Math.min(0,-0)",
	      -0,
	      Math.min(0,-0) );

new TestCase( SECTION,
	      "Math.min(-0,-0)",
	      -0,
	      Math.min(-0,-0) );

new TestCase( SECTION,
	      "Infinity/Math.min(0,-0)",
	      -Infinity,
	      Infinity/Math.min(0,-0) );

new TestCase( SECTION,
	      "Infinity/Math.min(-0,-0)",
	      -Infinity,
	      Infinity/Math.min(-0,-0) );

test();
