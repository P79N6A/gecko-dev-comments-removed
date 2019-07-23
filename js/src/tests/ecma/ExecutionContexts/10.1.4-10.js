





































gTestfile = '10.1.4-10.js';



































var SECTION = "10.1.4-10";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

new TestCase( "SECTION", "MYOBJECT.toString()" );

test();

function test() {
  for ( gTc=0; gTc < gTestcases.length; gTc++ ) {
    var VALUE = 12345;
    var MYOBJECT = new Number( VALUE );

    with ( MYOBJECT ) {
      gTestcases[gTc].actual = toString();
      gTestcases[gTc].expect = String(VALUE);
    }

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
