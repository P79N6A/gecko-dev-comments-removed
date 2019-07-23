





































gTestfile = '15.9.5.9.js';














var SECTION = "15.9.5.9";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCMonth()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCMonth()",
	      NaN,
	      (new Date(NaN)).getUTCMonth() );

new TestCase( SECTION,
	      "Date.prototype.getUTCMonth.length",
	      0,
	      Date.prototype.getUTCMonth.length );
test();

function addTestCase( t ) {
  var leap = InLeapYear(t);

  for ( var m = 0; m < 12; m++ ) {

    t += TimeInMonth(m, leap);

    new TestCase( SECTION,
		  "(new Date("+t+")).getUTCMonth()",
		  MonthFromTime(t),
		  (new Date(t)).getUTCMonth() );

    new TestCase( SECTION,
		  "(new Date("+(t+1)+")).getUTCMonth()",
		  MonthFromTime(t+1),
		  (new Date(t+1)).getUTCMonth() );

    new TestCase( SECTION,
		  "(new Date("+(t-1)+")).getUTCMonth()",
		  MonthFromTime(t-1),
		  (new Date(t-1)).getUTCMonth() );

    new TestCase( SECTION,
		  "(new Date("+(t-TZ_ADJUST)+")).getUTCMonth()",
		  MonthFromTime(t-TZ_ADJUST),
		  (new Date(t-TZ_ADJUST)).getUTCMonth() );

    new TestCase( SECTION,
		  "(new Date("+(t+TZ_ADJUST)+")).getUTCMonth()",
		  MonthFromTime(t+TZ_ADJUST),
		  (new Date(t+TZ_ADJUST)).getUTCMonth() );

  }
}
