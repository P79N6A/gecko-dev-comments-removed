


























var SECTION = "lexical-001";
var VERSION = "JS1_4";
var TITLE   = "Line Terminators";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = eval("\r\n\expect");
} catch ( e ) {
  exception = e.toString();
}

new TestCase(
  SECTION,
  "OBJECT = new Object; result = new OBJECT()" +
  " (threw " + exception +")",
  expect,
  result );

test();
