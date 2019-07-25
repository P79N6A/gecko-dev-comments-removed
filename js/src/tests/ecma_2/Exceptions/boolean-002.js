




















var SECTION = "boolean-002.js";
var VERSION = "JS1_4";
var TITLE   = "Boolean.prototype.valueOf()";
startTest();
writeHeaderToLog( SECTION +" "+ TITLE );


var exception = "No exception thrown";
var result = "Failed";

var VALUE_OF = Boolean.prototype.valueOf;

try {
  var s = new String("Not a Boolean");
  s.valueOf = VALUE_0F;
  s.valueOf();
} catch ( e ) {
  result = "Passed!";
  exception = e.toString();
}

new TestCase(
  SECTION,
  "Assigning Boolean.prototype.valueOf to a String object "+
  "(threw " +exception +")",
  "Passed!",
  result );

test();

