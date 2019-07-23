





































gTestfile = '15.9.5.19.js';














var SECTION = "15.9.5.19";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCSeconds()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCSeconds()",
	      NaN,
	      (new Date(NaN)).getUTCSeconds() );

new TestCase( SECTION,
	      "Date.prototype.getUTCSeconds.length",
	      0,
	      Date.prototype.getUTCSeconds.length );
test();

function addTestCase( t ) {
  for ( m = 0; m <= 60; m+=10 ) {
    t += 1000;
    new TestCase( SECTION,
		  "(new Date("+t+")).getUTCSeconds()",
		  SecFromTime(t),
		  (new Date(t)).getUTCSeconds() );
  }
}
