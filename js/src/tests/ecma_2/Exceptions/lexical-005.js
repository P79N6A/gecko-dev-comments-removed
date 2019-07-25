
























var SECTION = "lexical-005";
var VERSION = "JS1_4";
var TITLE   = "Keywords";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("true = false;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "true = false" +
  " (threw " + exception +")",
  expect,
  result );

test();

