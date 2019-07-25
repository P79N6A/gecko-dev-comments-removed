




















var SECTION = "date-002";
var VERSION = "JS1_4";
var TITLE   = "Date.prototype.setTime()";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var result = "Failed";
var exception = "No exception thrown";
var expect = "Passed";

try {
  var MYDATE = new MyDate();
  result = MYDATE.setTime(0);
} catch ( e ) {
  result = expect;
  exception = e.toString();
}

new TestCase(
  SECTION,
  "MYDATE = new MyDate(); MYDATE.setTime(0)" +
  " (threw " + exception +")",
  expect,
  result );

test();

function MyDate(value) {
  this.value = value;
  this.setTime = Date.prototype.setTime;
  return this;
}
