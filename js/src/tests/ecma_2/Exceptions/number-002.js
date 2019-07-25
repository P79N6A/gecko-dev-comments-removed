



















var SECTION = "number-002";
var VERSION = "JS1_4";
var TITLE   = "Exceptions for Number.valueOf()";

startTest();
writeHeaderToLog( SECTION + " Number.prototype.valueOf()");

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  object= new Object();
  object.toString = Number.prototype.valueOf;
  result = object.toString();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "object = new Object(); object.valueOf = Number.prototype.valueOf; object.valueOf()" +
  " (threw " + exception +")",
  expect,
  result );

test();
