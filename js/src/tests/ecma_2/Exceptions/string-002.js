




















var SECTION = "string-002";
var VERSION = "JS1_4";
var TITLE   = "String.prototype.valueOf";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var OBJECT =new Object();
  OBJECT.valueOf = String.prototype.valueOf;
  result = OBJECT.valueOf();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "OBJECT = new Object; OBJECT.valueOf = String.prototype.valueOf;"+
  "result = OBJECT.valueOf();" +
  " (threw " + exception +")",
  expect,
  result );

test();


