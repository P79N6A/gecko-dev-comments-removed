













var SECTION = "expression-017";
var VERSION = "JS1_4";
var TITLE   = "Function Calls";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = nullvalueOf();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "null.valueOf()" +
  " (threw " + exception +")",
  expect,
  result );

test();
