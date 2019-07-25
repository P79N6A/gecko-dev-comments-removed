














var SECTION = "expression-012";
var VERSION = "JS1_4";
var TITLE   = "The new operator";
var BUGNUMBER= "327765";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var STRING = new String("hi");
var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = new STRING();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "STRING = new String(\"hi\"); result = new STRING()" +
  " (threw " + exception +")",
  expect,
  result );

test();

