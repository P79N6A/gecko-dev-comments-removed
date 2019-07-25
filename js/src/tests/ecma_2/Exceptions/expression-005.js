















var SECTION = "expression-005";
var VERSION = "JS1_4";
var TITLE   = "The new operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var expect = "Passed";
var exception = "No exception thrown";

try {
  result = new Math();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "result= new Math() (threw " + exception + ")",
  expect,
  result );

test();
