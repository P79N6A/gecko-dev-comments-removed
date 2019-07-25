
























var SECTION = "lexical-013";
var VERSION = "JS1_4";
var TITLE   = "Future Reserved Words";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval("default = true;");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "default = true" +
  " (threw " + exception +")",
  expect,
  result );

test();


