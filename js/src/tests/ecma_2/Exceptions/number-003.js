



















var SECTION = "number-003";
var VERSION = "JS1_4";
var TITLE   = "Exceptions for Number.valueOf()";

startTest();
writeHeaderToLog( SECTION + " Number.prototype.valueOf()");

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  VALUE_OF = Number.prototype.valueOf;
  OBJECT = new String("Infinity");
  OBJECT.valueOf = VALUE_OF;
  result = OBJECT.valueOf();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "Assigning Number.prototype.valueOf as the valueOf of a String object " +
  " (threw " + exception +")",
  expect,
  result );

test();

