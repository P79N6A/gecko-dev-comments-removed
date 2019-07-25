



























































var SECTION = "10.5.1-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Global Object");


new TestCase( "SECTION", "Global Code check" );

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

test();

