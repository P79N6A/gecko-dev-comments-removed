













var SECTION = "expression-016";
var VERSION = "JS1_4";
var TITLE   = "Function Calls";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = (void 0).valueOf();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "(void 0).valueOf()" +
  " (threw " + exception +")",
  expect,
  result );

test();
