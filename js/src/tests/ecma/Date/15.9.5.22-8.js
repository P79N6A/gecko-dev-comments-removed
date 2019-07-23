





































gTestfile = '15.9.5.22-8.js';















var SECTION = "15.9.5.22";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getTimezoneOffset()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "(new Date(NaN)).getTimezoneOffset()",
	      NaN,
	      (new Date(NaN)).getTimezoneOffset() );

new TestCase( SECTION,
	      "Date.prototype.getTimezoneOffset.length",
	      0,
	      Date.prototype.getTimezoneOffset.length );
test();

