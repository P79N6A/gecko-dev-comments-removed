







































































var SECTION = "10.1.4-10";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

new TestCase( "SECTION", "MYOBJECT.toString()" );

test();

function test() {
  for ( tc=0; tc < testcases.length; tc++ ) {
    var VALUE = 12345;
    var MYOBJECT = new Number( VALUE );

    with ( MYOBJECT ) {
      testcases[tc].actual = toString();
      testcases[tc].expect = String(VALUE);
    }

    testcases[tc].passed = writeTestCaseResult(
      testcases[tc].expect,
      testcases[tc].actual,
      testcases[tc].description +" = "+
      testcases[tc].actual );

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
  }
  stopTest();
  return ( testcases );
}
