



















































var SECTION = "15.9.5.23-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.setTime()";

writeHeaderToLog( SECTION + " "+ TITLE);

var TZ_ADJUST = TZ_DIFF * msPerHour;


var now = (new Date()).valueOf();

test_times = new Array( now, TIME_1970, TIME_1900, TIME_2000 );


for ( var j = 0; j < test_times.length; j++ ) {
  addTestCase( new Date(now), test_times[j] );
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
