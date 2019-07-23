





































gTestfile = '15.9.5.4-2-n.js';














var SECTION = "15.9.5.4-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getTime";

writeHeaderToLog( SECTION + " "+ TITLE);

var MYDATE = new MyDate( TIME_2000 );

DESCRIPTION = "MYDATE.getTime()";
EXPECTED = "error";

new TestCase( SECTION,
	      "MYDATE.getTime()",
	      "error",
	      eval("MYDATE.getTime()") );

test();

function MyDate( value ) {
  this.value = value;
  this.getTime = Date.prototype.getTime;
}
