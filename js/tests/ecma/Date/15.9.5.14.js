

















































var SECTION = "15.9.5.14";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getHours()";

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
	      "(new Date(NaN)).getHours()",
	      NaN,
	      (new Date(NaN)).getHours() );

new TestCase( SECTION,
	      "Date.prototype.getHours.length",
	      0,
	      Date.prototype.getHours.length );
test();

function addTestCase( t ) {
  for ( h = 0; h < 24; h+=4 ) {
    t += msPerHour;
    new TestCase( SECTION,
		  "(new Date("+t+")).getHours()",
		  HourFromTime((LocalTime(t))),
		  (new Date(t)).getHours() );
  }
}
