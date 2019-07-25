













var SECTION = "expression-010";
var VERSION = "JS1_4";
var TITLE   = "The new operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var NUMBER = 0;

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = new NUMBER();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "NUMBER=0, result = new NUMBER()" +
  " (threw " + exception +")",
  expect,
  result );

test();

