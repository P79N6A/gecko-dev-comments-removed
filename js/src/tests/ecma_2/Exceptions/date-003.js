




















var SECTION = "date-003";
var VERSION = "JS1_4";
var TITLE   = "Date.prototype.valueOf";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var OBJ = new MyObject( new Date(0) );
  result = OBJ.valueOf();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "OBJ = new MyObject( new Date(0)); OBJ.valueOf()" +
  " (threw " + exception +")",
  expect,
  result );

test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = Date.prototype.valueOf;


  return this;
}
