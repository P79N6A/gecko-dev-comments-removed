





































gTestfile = '15.9.5.22-1.js';















var SECTION = "15.9.5.22";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getTimezoneOffset()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getTimezoneOffset()",
	      NaN,
	      (new Date(NaN)).getTimezoneOffset() );

new TestCase( SECTION,
	      "Date.prototype.getTimezoneOffset.length",
	      0,
	      Date.prototype.getTimezoneOffset.length );

test();

function addTestCase( t ) {
  for ( m = 0; m <= 1000; m+=100 ) {
    t++;
    new TestCase( SECTION,
		  "(new Date("+t+")).getTimezoneOffset()",
		  (t - LocalTime(t)) / msPerMinute,
		  (new Date(t)).getTimezoneOffset() );
  }
}
