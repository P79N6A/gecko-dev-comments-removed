


















































var SECTION = "15.9.5.13";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCDay()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;

addTestCase( TIME_1970 );

test();

function addTestCase( t ) {
  for ( var m = 0; m < 12; m++ ) {
    t += TimeInMonth(m);

    for ( d = 0; d < TimeInMonth(m); d+= msPerDay*14 ) {
      t += d;

      new TestCase( SECTION,
		    "(new Date("+t+")).getUTCDay()",
		    WeekDay((t)),
		    (new Date(t)).getUTCDay() );
    }
  }
}
