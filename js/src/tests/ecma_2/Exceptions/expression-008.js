













var SECTION = "expression-008";
var VERSION = "JS1_4";
var TITLE   = "The new operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var NULL = null;
var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = new NULL();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "NULL = null; result = new NULL()" +
  " (threw " + exception +")",
  expect,
  result );

test();
