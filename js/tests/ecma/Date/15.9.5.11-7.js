


















































var SECTION = "15.9.5.11";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCDate()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;

var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

addTestCase( UTC_JAN_1_2005 );

test();

function addTestCase( t ) {
  for ( var m = 0; m < 11; m++ ) {
    t += TimeInMonth(m);

    for ( var d = 0; d < TimeInMonth( m ); d += 7*msPerDay ) {
      t += d;
      new TestCase( SECTION,
		    "(new Date("+t+")).getUTCDate()",
		    DateFromTime((t)),
		    (new Date(t)).getUTCDate() );





















    }
  }
}
