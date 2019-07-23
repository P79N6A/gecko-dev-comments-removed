



















































var SECTION = "15.9.5.22";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getTimezoneOffset()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;


var now = (new Date()).valueOf();


for ( var time = 0, year = 1969; year >= 0; year-- ) {
  time -= TimeInYear(year);
}


var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;



var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);



addTestCase( time );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getTimezoneOffset()",
	      NaN,
	      (new Date(NaN)).getTimezoneOffset() );

new TestCase( SECTION,
	      "Date.prototype.getTimezoneOffset.length",
	      0,
	      Date.prototype.getTimezoneOffset.length );

test();

function addTestCase( t ) {
  for ( m = 0; m <= 1000; m+=100 ) {
    t++;
    new TestCase( SECTION,
		  "(new Date("+t+")).getTimezoneOffset()",
		  (t - LocalTime(t)) / msPerMinute,
		  (new Date(t)).getTimezoneOffset() );
  }
}
