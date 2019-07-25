













var SECTION = "expression-009";
var VERSION = "JS1_4";
var TITLE   = "The new operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var STRING = "";

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
  "STRING = ''; result = new STRING()" +
  " (threw " + exception +")",
  expect,
  result );

test();
