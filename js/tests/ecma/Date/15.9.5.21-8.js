





































gTestfile = '15.9.5.21-8.js';














var SECTION = "15.9.5.21";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCMilliseconds()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCMilliseconds()",
	      NaN,
	      (new Date(NaN)).getUTCMilliseconds() );

new TestCase( SECTION,
	      "Date.prototype.getUTCMilliseconds.length",
	      0,
	      Date.prototype.getUTCMilliseconds.length );
test();

