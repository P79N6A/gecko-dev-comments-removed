














var SECTION = "statement-007";
var VERSION = "JS1_4";
var TITLE   = "The continue statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("continue;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "continue outside of an iteration statement" +
  " (threw " + exception +")",
  expect,
  result );

test();

