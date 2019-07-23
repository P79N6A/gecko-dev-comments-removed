





































gTestfile = '15.9.1.1-1.js';




































var FOUR_HUNDRED_YEARS = 1.26227808e+13;
var SECTION         =  "15.9.1.1-1";

writeHeaderToLog("15.9.1.1 Time Range");

var M_SECS;
var CURRENT_YEAR;

for ( M_SECS = 0, CURRENT_YEAR = 1970;
      M_SECS < 8640000000000000;
      M_SECS += FOUR_HUNDRED_YEARS, CURRENT_YEAR += 400 ) {

  new TestCase( SECTION,  
		"new Date("+M_SECS+")",
		CURRENT_YEAR,
		(new Date( M_SECS)).getUTCFullYear() );
}

test();

