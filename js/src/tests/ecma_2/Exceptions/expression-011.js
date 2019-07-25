













var SECTION = "expression-011";
var VERSION = "JS1_4";
var TITLE   = "The new operator";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var BOOLEAN  = true;

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var OBJECT = new BOOLEAN();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "BOOLEAN = true; result = new BOOLEAN()" +
  " (threw " + exception +")",
  expect,
  result );

test();

