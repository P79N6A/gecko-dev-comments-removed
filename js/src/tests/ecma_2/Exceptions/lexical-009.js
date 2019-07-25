
























var SECTION = "lexical-009";
var VERSION = "ECMA_1";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("debugger = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "debugger = true" +
  " (threw " + exception +")",
  expect,
  result );

test();


