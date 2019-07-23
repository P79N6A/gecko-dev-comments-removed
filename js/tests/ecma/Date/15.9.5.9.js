


















































var SECTION = "15.9.5.8";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCMonth()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;


var now = (new Date()).valueOf();


for ( var time = 0, year = 1969; year >= 0; year-- ) {
  time -= TimeInYear(year);
}


var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;



var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

addTestCase( now );
addTestCase( time );
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
  for ( var m = 0; m < 12; m++ ) {

    t += TimeInMonth(m);

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
