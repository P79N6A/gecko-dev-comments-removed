























var SECTION = "lexical-023.js";
var VERSION = "ECMA_1";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("try = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "try = true" +
  " (threw " + exception +")",
  expect,
  result );

test();


