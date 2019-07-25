














var SECTION = "expression-004";
var VERSION = "JS1_4";
var TITLE   = "Property Accessors";
writeHeaderToLog( SECTION + " "+TITLE );
startTest();

var OBJECT = new Property( "null", null, "null", 0 );

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
  "Get the toString value of an object whose value is null "+
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
