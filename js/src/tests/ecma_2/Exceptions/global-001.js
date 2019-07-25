


















var SECTION = "global-001";
var VERSION = "ECMA_1";
var TITLE   = "The Global Object";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = new this();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "result = new this()" +
  " (threw " + exception +")",
  expect,
  result );

test();
