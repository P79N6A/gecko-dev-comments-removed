





































gTestfile = '15.9.5.7.js';













var SECTION = "15.9.5.7";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCFullYear()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
	      "(new Date(NaN)).getUTCFullYear()",
	      NaN,
	      (new Date(NaN)).getUTCFullYear() );

new TestCase( SECTION,
	      "Date.prototype.getUTCFullYear.length",
	      0,
	      Date.prototype.getUTCFullYear.length );

test();

function addTestCase( t ) {
  new TestCase( SECTION,
		"(new Date("+t+")).getUTCFullYear()",
		YearFromTime(t),
		(new Date(t)).getUTCFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t+1)+")).getUTCFullYear()",
		YearFromTime(t+1),
		(new Date(t+1)).getUTCFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t-1)+")).getUTCFullYear()",
		YearFromTime(t-1),
		(new Date(t-1)).getUTCFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t-TZ_ADJUST)+")).getUTCFullYear()",
		YearFromTime(t-TZ_ADJUST),
		(new Date(t-TZ_ADJUST)).getUTCFullYear() );

  new TestCase( SECTION,
		"(new Date("+(t+TZ_ADJUST)+")).getUTCFullYear()",
		YearFromTime(t+TZ_ADJUST),
		(new Date(t+TZ_ADJUST)).getUTCFullYear() );
}
