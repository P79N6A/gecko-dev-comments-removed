


















































var SECTION = "15.9.5.13";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCDay()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;


var now = (new Date()).valueOf();


for ( var time = 0, year = 1969; year >= 0; year-- ) {
  time -= TimeInYear(year);
}


var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;



var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCDay()",
	      NaN,
	      (new Date(NaN)).getUTCDay() );

new TestCase( SECTION,
	      "Date.prototype.getUTCDay.length",
	      0,
	      Date.prototype.getUTCDay.length );

test();

function addTestCase( t ) {
  for ( var m = 0; m < 12; m++ ) {
    t += TimeInMonth(m);

    for ( d = 0; d < TimeInMonth(m); d+= msPerDay*7 ) {
      t += d;

      new TestCase( SECTION,
		    "(new Date("+t+")).getUTCDay()",
		    WeekDay((t)),
		    (new Date(t)).getUTCDay() );
    }
  }
}
