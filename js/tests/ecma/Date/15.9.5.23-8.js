





































gTestfile = '15.9.5.23-8.js';
















var SECTION = "15.9.5.23-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.setTime()";

writeHeaderToLog( SECTION + " "+ TITLE);

test_times = new Array( TIME_NOW, TIME_0000, TIME_1970, TIME_1900, TIME_2000,
			UTC_FEB_29_2000, UTC_JAN_1_2005 );


for ( var j = 0; j < test_times.length; j++ ) {
  addTestCase( new Date(UTC_FEB_29_2000), test_times[j] );
}

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
