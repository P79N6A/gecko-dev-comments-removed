


















var SECTION = "global-002";
var VERSION = "JS1_4";
var TITLE   = "The Global Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = this();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "result = this()" +
  " (threw " + exception +")",
  expect,
  result );

test();
