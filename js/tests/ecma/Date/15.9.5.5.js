





































gTestfile = '15.9.5.5.js';


















var SECTION = "15.9.5.5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getYear()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );

new TestCase( SECTION,
	      "(new Date(NaN)).getYear()",
	      NaN,
	      (new Date(NaN)).getYear() );

new TestCase( SECTION,
	      "Date.prototype.getYear.length",
	      0,
	      Date.prototype.getYear.length );

test();

function addTestCase( t ) {
  new TestCase( SECTION,
		"(new Date("+t+")).getYear()",
		GetYear(YearFromTime(LocalTime(t))),
		(new Date(t)).getYear() );

  new TestCase( SECTION,
		"(new Date("+(t+1)+")).getYear()",
		GetYear(YearFromTime(LocalTime(t+1))),
		(new Date(t+1)).getYear() );

  new TestCase( SECTION,
		"(new Date("+(t-1)+")).getYear()",
		GetYear(YearFromTime(LocalTime(t-1))),
		(new Date(t-1)).getYear() );

  new TestCase( SECTION,
		"(new Date("+(t-TZ_ADJUST)+")).getYear()",
		GetYear(YearFromTime(LocalTime(t-TZ_ADJUST))),
		(new Date(t-TZ_ADJUST)).getYear() );

  new TestCase( SECTION,
		"(new Date("+(t+TZ_ADJUST)+")).getYear()",
		GetYear(YearFromTime(LocalTime(t+TZ_ADJUST))),
		(new Date(t+TZ_ADJUST)).getYear() );
}
function GetYear( year ) {
  return year - 1900;
}
