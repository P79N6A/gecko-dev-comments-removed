







































































var SECTION = "10.1.4-9";
var VERSION = "ECMA_2";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

new TestCase( SECTION, "NEW_PROPERTY =  " );

test();

function test() {
  for ( tc=0; tc < testcases.length; tc++ ) {

    var MYOBJECT = new MyObject();
    var RESULT   = "hello";

    with ( MYOBJECT ) {
      NEW_PROPERTY = RESULT;
    }
    testcases[tc].actual = NEW_PROPERTY;
    testcases[tc].expect = RESULT;

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
function MyObject( n ) {
  this.__proto__ = Number.prototype;
}
