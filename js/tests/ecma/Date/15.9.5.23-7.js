




















































var SECTION = "15.9.5.23-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.setTime()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;


var now = (new Date()).valueOf();


for ( var time = 0, year = 1969; year >= 0; year-- ) {
  time -= TimeInYear(year);
}


var UTC_FEB_29_2000 = TIME_2000 + 31*msPerDay + 28*msPerHour;



var UTC_JAN_1_2005 = TIME_2000 + TimeInYear(2000)+TimeInYear(2001)+
TimeInYear(2002)+TimeInYear(2003)+TimeInYear(2004);

test_times = new Array( now, time, TIME_1970, TIME_1900, TIME_2000,
			UTC_FEB_29_2000, UTC_JAN_1_2005 );


for ( var j = 0; j < test_times.length; j++ ) {
  addTestCase( new Date(TIME_2000), test_times[j] );
}


new TestCase( SECTION,
	      "(new Date(NaN)).setTime()",
	      NaN,
	      (new Date(NaN)).setTime() );

new TestCase( SECTION,
	      "Date.prototype.setTime.length",
	      1,
	      Date.prototype.setTime.length );
test();

function addTestCase( d, t ) {
  new TestCase( SECTION,
		"( "+d+" ).setTime("+t+")",
		t,
		d.setTime(t) );

  new TestCase( SECTION,
		"( "+d+" ).setTime("+(t+1.1)+")",
		TimeClip(t+1.1),
		d.setTime(t+1.1) );

  new TestCase( SECTION,
		"( "+d+" ).setTime("+(t+1)+")",
		t+1,
		d.setTime(t+1) );

  new TestCase( SECTION,
		"( "+d+" ).setTime("+(t-1)+")",
		t-1,
		d.setTime(t-1) );

  new TestCase( SECTION,
		"( "+d+" ).setTime("+(t-TZ_ADJUST)+")",
		t-TZ_ADJUST,
		d.setTime(t-TZ_ADJUST) );

  new TestCase( SECTION,
		"( "+d+" ).setTime("+(t+TZ_ADJUST)+")",
		t+TZ_ADJUST,
		d.setTime(t+TZ_ADJUST) );
}
