





































gTestfile = '15.9.5.4-1.js';












var SECTION = "15.9.5.4-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getTime";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

test();

function addTestCase( t ) {
  new TestCase( SECTION,
		"(new Date("+t+").getTime()",
		t,
		(new Date(t)).getTime() );

  new TestCase( SECTION,
		"(new Date("+(t+1)+").getTime()",
		t+1,
		(new Date(t+1)).getTime() );

  new TestCase( SECTION,
		"(new Date("+(t-1)+").getTime()",
		t-1,
		(new Date(t-1)).getTime() );

  new TestCase( SECTION,
		"(new Date("+(t-TZ_ADJUST)+").getTime()",
		t-TZ_ADJUST,
		(new Date(t-TZ_ADJUST)).getTime() );

  new TestCase( SECTION,
		"(new Date("+(t+TZ_ADJUST)+").getTime()",
		t+TZ_ADJUST,
		(new Date(t+TZ_ADJUST)).getTime() );
}
