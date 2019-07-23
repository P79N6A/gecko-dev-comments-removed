





































gTestfile = '15.9.5.23-3-n.js';
















var SECTION = "15.9.5.23-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.setTime()";

writeHeaderToLog( SECTION + " "+ TITLE);

var MYDATE = new MyDate(TIME_1970);

DESCRIPTION = "MYDATE.setTime(TIME_2000)";
EXPECTED = "error";

new TestCase( SECTION,
	      "MYDATE.setTime(TIME_2000)",
	      "error",
	      eval("MYDATE.setTime(TIME_2000)") );

test();

function MyDate(value) {
  this.value = value;
  this.setTime = Date.prototype.setTime;
  return this;
}
