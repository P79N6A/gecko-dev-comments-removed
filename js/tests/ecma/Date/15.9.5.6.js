





































gTestfile = '15.9.5.6.js';













var SECTION = "15.9.5.6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getFullYear()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getFullYear()",
	      NaN,
	      (new Date(NaN)).getFullYear() );

new TestCase( SECTION,
	      "Date.prototype.getFullYear.length",
	      0,
	      Date.prototype.getFullYear.length );

test();
function addTestCase( t ) {
  new TestCase( SECTION,
		"(new Date("+t+")).getFullYear()",
		YearFromTime(LocalTime(t)),
		(new Date(t)).getFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t+1)+")).getFullYear()",
		YearFromTime(LocalTime(t+1)),
		(new Date(t+1)).getFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t-1)+")).getFullYear()",
		YearFromTime(LocalTime(t-1)),
		(new Date(t-1)).getFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t-TZ_ADJUST)+")).getFullYear()",
		YearFromTime(LocalTime(t-TZ_ADJUST)),
		(new Date(t-TZ_ADJUST)).getFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t+TZ_ADJUST)+")).getFullYear()",
		YearFromTime(LocalTime(t+TZ_ADJUST)),
		(new Date(t+TZ_ADJUST)).getFullYear() );
}
