






























var SECTION = "lexical-024";
var VERSION = "JS1_4";
var TITLE   = "Keywords";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("var break;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "var break" +
  " (threw " + exception +")",
  expect,
  result );

test();


