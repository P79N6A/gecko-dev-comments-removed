














var SECTION = "expression-014.js";
var VERSION = "ECMA_1";
var TITLE   = "The new operator";
var BUGNUMBER= "327765";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var BOOLEAN = new Boolean();


var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = new BOOLEAN();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "BOOLEAN = new Boolean(); result = new BOOLEAN()" +
  " (threw " + exception +")",
  expect,
  result );

test();

