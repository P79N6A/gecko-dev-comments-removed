
















var SECTION = "expression-006.js";
var VERSION = "JS1_4";
var TITLE   = "The new operator";
var BUGNUMBER="327765";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var OBJECT = new Object();
  result = new OBJECT();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "OBJECT = new Object; result = new OBJECT()" +
  " (threw " + exception +")",
  expect,
  result );

test();

