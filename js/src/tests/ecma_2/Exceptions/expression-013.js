













var SECTION = "expression-013";
var VERSION = "JS1_4";
var TITLE   = "The new operator";
var BUGNUMBER= "327765";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var NUMBER = new Number(1);

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
  "NUMBER = new Number(1); result = new NUMBER()" +
  " (threw " + exception +")",
  expect,
  result );

test();

