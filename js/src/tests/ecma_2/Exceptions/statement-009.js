













var SECTION = "12.9-1-n";
var VERSION = "ECMA_1";
var TITLE   = "The return statement";

startTest();
writeHeaderToLog( SECTION + " The return statement");

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("return;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "return outside of a function" +
  " (threw " + exception +")",
  expect,
  result );

test();

