
















var SECTION = "expressions-003.js";
var VERSION = "JS1_4";
var TITLE   = "Property Accessors";
writeHeaderToLog( SECTION + " "+TITLE );

startTest();



OBJECT = new Property(  "undefined",    void 0,   "undefined",   NaN );

var result    = "Failed";
var exception = "No exception thrown";
var expect    = "Passed";

try {
  result = OBJECT.value.toString();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}


new TestCase(
  SECTION,
  "Get the toString value of an object whose value is undefined "+
  "(threw " + exception +")",
  expect,
  result );

test();

function Property( object, value, string, number ) {
  this.object = object;
  this.string = String(value);
  this.number = Number(value);
  this.value = value;
}
