





































gTestfile = '10.1.4-9.js';



































var SECTION = "10.1.4-9";
var VERSION = "ECMA_2";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

new TestCase( SECTION, "NEW_PROPERTY =  " );

test();

function test() {
  for ( gTc=0; gTc < gTestcases.length; gTc++ ) {

    var MYOBJECT = new MyObject();
    var RESULT   = "hello";

    with ( MYOBJECT ) {
      NEW_PROPERTY = RESULT;
    }
    gTestcases[gTc].actual = NEW_PROPERTY;
    gTestcases[gTc].expect = RESULT;

    gTestcases[gTc].passed = writeTestCaseResult(
      gTestcases[gTc].expect,
      gTestcases[gTc].actual,
      gTestcases[gTc].description +" = "+
      gTestcases[gTc].actual );

    gTestcases[gTc].reason += ( gTestcases[gTc].passed ) ? "" : "wrong value ";
  }
  stopTest();
  return ( gTestcases );
}
function MyObject( n ) {
  this.__proto__ = Number.prototype;
}
