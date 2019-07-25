

















var SECTION = "lexical-039";
var VERSION = "JS1_4";
var TITLE   = "Identifiers";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("var 0abc;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "var 0abc" +
  " (threw " + exception +")",
  expect,
  result );

test();


