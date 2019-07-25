
























var SECTION = "lexical-008.js";
var VERSION = "JS1_4";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("case = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "case = true" +
  " (threw " + exception +")",
  expect,
  result );

test();


