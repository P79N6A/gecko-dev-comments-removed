






























var SECTION = "lexical-033";
var VERSION = "JS1_4";
var TITLE   = "Keywords";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("if = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "if = true" +
  " (threw " + exception +")",
  expect,
  result );

test();


