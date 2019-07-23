





































gTestfile = '15.9.5.17.js';














var SECTION = "15.9.5.17";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCMinutes()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCMinutes()",
	      NaN,
	      (new Date(NaN)).getUTCMinutes() );

new TestCase( SECTION,
	      "Date.prototype.getUTCMinutes.length",
	      0,
	      Date.prototype.getUTCMinutes.length );
test();

function addTestCase( t ) {
  for ( m = 0; m <= 60; m+=10 ) {
    t += msPerMinute;
    new TestCase( SECTION,
		  "(new Date("+t+")).getUTCMinutes()",
		  MinFromTime(t),
		  (new Date(t)).getUTCMinutes() );
  }
}
