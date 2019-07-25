
















var SECTION = "expressions-002.js";
var VERSION = "JS1_4";
var TITLE   = "Property Accessors";
writeHeaderToLog( SECTION + " "+TITLE );

startTest();



var PROPERTY = new Array();
var p = 0;



OBJECT = new Property(  "undefined",    void 0,   "undefined",   NaN );

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  result = OBJECT.value.valueOf();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}


new TestCase(
  SECTION,
  "Get the value of an object whose value is undefined "+
  "(threw " + exception +")",
  expect,
  result );

test();

function Property( object, value, string, number ) {
  this.object = object;
  this.string = String(value);
  this.number = Number(value);
  this.valueOf = value;
}
