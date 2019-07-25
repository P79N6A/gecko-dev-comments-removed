






















var SECTION = "string-001";
var VERSION = "JS1_4";
var TITLE   = "String.prototype.toString";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  OBJECT = new Object();
  OBJECT.toString = String.prototype.toString();
  result = OBJECT.toString();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "OBJECT = new Object; "+
  " OBJECT.toString = String.prototype.toString; OBJECT.toString()" +
  " (threw " + exception +")",
  expect,
  result );

test();

