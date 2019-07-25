















var SECTION = "expression-007";
var VERSION = "JS1_4";
var TITLE   = "The new operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  UNDEFINED = void 0;
  result = new UNDEFINED();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "UNDEFINED = void 0; result = new UNDEFINED()" +
  " (threw " + exception +")",
  expect,
  result );

test();

