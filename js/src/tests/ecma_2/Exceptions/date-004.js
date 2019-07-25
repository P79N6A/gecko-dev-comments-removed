

















var SECTION = "date-004";
var VERSION = "JS1_4";
var TITLE   = "Date.prototype.getTime";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var MYDATE = new MyDate();
  result = MYDATE.getTime();
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "MYDATE = new MyDate(); MYDATE.getTime()" +
  " (threw " + exception +")",
  expect,
  result );

test();

function MyDate( value ) {
  this.value = value;
  this.getTime = Date.prototype.getTime;
}
