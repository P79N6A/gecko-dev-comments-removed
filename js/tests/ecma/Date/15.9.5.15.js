





































gTestfile = '15.9.5.15.js';














var SECTION = "15.9.5.15";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCHours()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCHours()",
	      NaN,
	      (new Date(NaN)).getUTCHours() );

new TestCase( SECTION,
	      "Date.prototype.getUTCHours.length",
	      0,
	      Date.prototype.getUTCHours.length );
test();

function addTestCase( t ) {
  for ( h = 0; h < 24; h+=3 ) {
    t += msPerHour;
    new TestCase( SECTION,
		  "(new Date("+t+")).getUTCHours()",
		  HourFromTime((t)),
		  (new Date(t)).getUTCHours() );
  }
}
