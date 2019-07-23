




































gTestfile = 'date-001.js';





















var SECTION = "date-001";
var VERSION = "JS1_4";
var TITLE   = "Date.prototype.toString";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var OBJ = new MyObject( new Date(0) );
  result = OBJ.toString();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "OBJECT = new MyObject( new Date(0)) ; result = OBJ.toString()" +
  " (threw " + exception +")",
  expect,
  result );

test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
  this.toString = Date.prototype.toString;
  return this;
}
