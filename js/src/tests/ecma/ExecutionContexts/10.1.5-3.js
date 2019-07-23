





































gTestfile = '10.1.5-3.js';






















var SECTION = "10.5.1-3";
var VERSION = "ECMA_1";
startTest();
writeHeaderToLog( SECTION + " Global Object");

new TestCase( "SECTION", "Function Code check" );

test();

function test() {
  if ( Object == null ) {
    gTestcases[0].reason += " Object == null" ;
  }
  if ( Function == null ) {
    gTestcases[0].reason += " Function == null";
  }
  if ( String == null ) {
    gTestcases[0].reason += " String == null";
  }
  if ( Array == null ) {
    gTestcases[0].reason += " Array == null";
  }
  if ( Number == null ) {
    gTestcases[0].reason += " Function == null";
  }
  if ( Math == null ) {
    gTestcases[0].reason += " Math == null";
  }
  if ( Boolean == null ) {
    gTestcases[0].reason += " Boolean == null";
  }
  if ( Date  == null ) {
    gTestcases[0].reason += " Date == null";
  }








  if ( eval == null ) {
    gTestcases[0].reason += " eval == null";
  }
  if ( parseInt == null ) {
    gTestcases[0].reason += " parseInt == null";
  }

  if ( gTestcases[0].reason != "" ) {
    gTestcases[0].actual = "fail";
  } else {
    gTestcases[0].actual = "pass";
  }
  gTestcases[0].expect = "pass";

  for ( gTc=0; gTc < gTestcases.length; gTc++ ) {

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
