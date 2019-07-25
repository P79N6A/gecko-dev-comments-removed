

















var SECTION = "boolean-001.js";
var VERSION = "JS1_4";
var TITLE   = "Boolean.prototype.toString()";
startTest();
writeHeaderToLog( SECTION +" "+ TITLE );

var exception = "No exception thrown";
var result = "Failed";

var TO_STRING = Boolean.prototype.toString;

try {
  var s = new String("Not a Boolean");
  s.toString = TO_STRING;
  s.toString();
} catch ( e ) {
  result = "Passed!";
  exception = e.toString();
}

new TestCase(
  SECTION,
  "Assigning Boolean.prototype.toString to a String object "+
  "(threw " +exception +")",
  "Passed!",
  result );

test();

