













var SECTION = "expression-015";
var VERSION = "JS1_4";
var TITLE   = "Function Calls";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("result = 3.valueOf();");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "3.valueOf()" +
  " (threw " + exception +")",
  expect,
  result );

test();
