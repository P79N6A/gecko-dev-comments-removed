





































gTestfile = '15.9.5.20.js';














var SECTION = "15.9.5.20";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getMilliseconds()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getMilliseconds()",
	      NaN,
	      (new Date(NaN)).getMilliseconds() );

new TestCase( SECTION,
	      "Date.prototype.getMilliseconds.length",
	      0,
	      Date.prototype.getMilliseconds.length );
test();

function addTestCase( t ) {
  for ( m = 0; m <= 1000; m+=100 ) {
    t++;
    new TestCase( SECTION,
		  "(new Date("+t+")).getMilliseconds()",
		  msFromTime(LocalTime(t)),
		  (new Date(t)).getMilliseconds() );
  }
}
