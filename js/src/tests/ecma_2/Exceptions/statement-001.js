



















var SECTION = "statement-001.js";

var VERSION = "ECMA_1";
var TITLE   = "The for statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("for (i) {\n}");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "for(i) {}" +
  " (threw " + exception +")",
  expect,
  result );

test();
