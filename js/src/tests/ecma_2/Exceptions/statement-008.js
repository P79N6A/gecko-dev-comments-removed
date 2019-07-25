














var SECTION = "statement-008";
var VERSION = "JS1_4";
var TITLE   = "The break in statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("break;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "break outside of an iteration statement" +
  " (threw " + exception +")",
  expect,
  result );

test();

