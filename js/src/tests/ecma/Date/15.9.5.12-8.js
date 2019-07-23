





































gTestfile = '15.9.5.12-8.js';















var SECTION = "15.9.5.12";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getDay()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "(new Date(NaN)).getDay()",
	      NaN,
	      (new Date(NaN)).getDay() );

new TestCase( SECTION,
	      "Date.prototype.getDay.length",
	      0,
	      Date.prototype.getDay.length );
test();
