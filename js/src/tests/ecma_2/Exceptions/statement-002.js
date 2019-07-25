










































var SECTION = "statement-002";
var VERSION = "JS1_4";
var TITLE   = "The for..in statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  eval(" for ( var i, p in this) { result += this[p]; }");
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "more than one member expression" +
  " (threw " + exception +")",
  expect,
  result );

test();
