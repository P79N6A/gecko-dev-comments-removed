


















































var SECTION = "15.9.5.10";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getDate()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;


var now = (new Date()).valueOf();


for ( var time = 0, year = 1969; year >= 0; year-- ) {
  time -= TimeInYear(year);
}


var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;



var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);



var DST_START_1998 = UTC( GetFirstSundayInApril(TimeFromYear(1998)) + 2*msPerHour )

  var DST_END_1998 = UTC( GetLastSundayInOctober(TimeFromYear(1998)) + 2*msPerHour );

addTestCase( UTC_JAN_1_2005 );









new TestCase( SECTION,
	      "(new Date(NaN)).getDate()",
	      NaN,
	      (new Date(NaN)).getDate() );

new TestCase( SECTION,
	      "Date.prototype.getDate.length",
	      0,
	      Date.prototype.getDate.length );
test();

function addTestCase( t ) {
  for ( d = 0; d < TimeInMonth(MonthFromTime(t)); d+= msPerDay ) {
    t += d;
    testcases[tc++] = new TestCase( SECTION,
                                    "(new Date("+t+")).getDate()",
                                    DateFromTime(LocalTime(t)),
                                    (new Date(t)).getDate() );
  }
}
