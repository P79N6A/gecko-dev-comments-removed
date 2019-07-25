
























var SECTION = "number-001";
var VERSION = "JS1_4";
var TITLE   = "Exceptions for Number.toString()";

startTest();
writeHeaderToLog( SECTION + " Number.prototype.toString()");

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  object= new Object();
  object.toString = Number.prototype.toString;
  result = object.toString();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "object = new Object(); object.toString = Number.prototype.toString; object.toString()" +
  " (threw " + exception +")",
  expect,
  result );

test();
