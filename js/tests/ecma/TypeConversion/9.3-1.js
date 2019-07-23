



























































var SECTION = "9.3-1";
var VERSION = "ECMA_1";
startTest();
var TYPE = "number";

writeHeaderToLog( SECTION + " ToNumber");


new TestCase( SECTION,   "Number(new Number())",          0,              Number(new Number())  );
new TestCase( SECTION,   "Number(new Number(Number.NaN))",Number.NaN,     Number(new Number(Number.NaN)) );
new TestCase( SECTION,   "Number(new Number(0))",         0,              Number(new Number(0)) );
new TestCase( SECTION,   "Number(new Number(null))",      0,              Number(new Number(null)) );

new TestCase( SECTION,   "Number(new Number(true))",      1,              Number(new Number(true)) );
new TestCase( SECTION,   "Number(new Number(false))",     0,              Number(new Number(false)) );



new TestCase( SECTION,   "Number(new Boolean(true))",     1,  Number(new Boolean(true)) );
new TestCase( SECTION,   "Number(new Boolean(false))",    0,  Number(new Boolean(false)) );


new TestCase( SECTION,   "Number(new Array(2,4,8,16,32))",      Number.NaN,     Number(new Array(2,4,8,16,32)) );




test();


function test() {
  for ( tc=0; tc < testcases.length; tc++ ) {
    testcases[tc].passed = writeTestCaseResult(
      testcases[tc].expect,
      testcases[tc].actual,
      testcases[tc].description +" = "+
      testcases[tc].actual );

    testcases[tc].passed = writeTestCaseResult(
      TYPE,
      typeof(testcases[tc].actual),
      "typeof( " + testcases[tc].description +
      " ) = " + typeof(testcases[tc].actual) )
      ? testcases[tc].passed
      : false;

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
  }
  stopTest();
  return ( testcases );
}
