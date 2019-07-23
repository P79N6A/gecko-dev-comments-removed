





































gTestfile = '15.9.3.1-1.js';



































var SECTION = "15.9.3.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "new Date( year, month, date, hours, minutes, seconds, ms )";

var TIME        = 0;
var UTC_YEAR    = 1;
var UTC_MONTH   = 2;
var UTC_DATE    = 3;
var UTC_DAY     = 4;
var UTC_HOURS   = 5;
var UTC_MINUTES = 6;
var UTC_SECONDS = 7;
var UTC_MS      = 8;

var YEAR        = 9;
var MONTH       = 10;
var DATE        = 11;
var DAY         = 12;
var HOURS       = 13;
var MINUTES     = 14;
var SECONDS     = 15;
var MS          = 16;

writeHeaderToLog( SECTION + " "+ TITLE);



addNewTestCase( new Date( 1969,11,31,15,59,59,999),
		"new Date( 1969,11,31,15,59,59,999)",
		[TIME_1970-1,1969,11,31,3,23,59,59,999,1969,11,31,3,15,59,59,999] );

addNewTestCase( new Date( 1969,11,31,23,59,59,999),
		"new Date( 1969,11,31,23,59,59,999)",
		[TIME_1970-PST_ADJUST-1,1970,0,1,4,7,59,59,999,1969,11,31,3,23,59,59,999] );

addNewTestCase( new Date( 1970,0,1,0,0,0,0),
		"new Date( 1970,0,1,0,0,0,0)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

addNewTestCase( new Date( 1969,11,31,16,0,0,0),
		"new Date( 1969,11,31,16,0,0,0)",
		[TIME_1970,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

addNewTestCase( new Date(1969,12,1,0,0,0,0),
		"new Date(1969,12,1,0,0,0,0)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

addNewTestCase( new Date(1969,11,32,0,0,0,0),
		"new Date(1969,11,32,0,0,0,0)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

addNewTestCase( new Date(1969,11,31,24,0,0,0),
		"new Date(1969,11,31,24,0,0,0)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

addNewTestCase( new Date(1969,11,31,23,60,0,0),
		"new Date(1969,11,31,23,60,0,0)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

addNewTestCase( new Date(1969,11,31,23,59,60,0),
		"new Date(1969,11,31,23,59,60,0)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );

addNewTestCase( new Date(1969,11,31,23,59,59,1000),
		"new Date(1969,11,31,23,59,59,1000)",
		[TIME_1970-PST_ADJUST,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );



addNewTestCase( new Date( 1999,11,31,15,59,59,999),
		"new Date( 1999,11,31,15,59,59,999)",
		[TIME_2000-1,1999,11,31,5,23,59,59,999,1999,11,31,5,15,59,59,999] );

addNewTestCase( new Date( 1999,11,31,16,0,0,0),
		"new Date( 1999,11,31,16,0,0,0)",
		[TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5, 16,0,0,0] );

addNewTestCase( new Date( 1999,11,31,23,59,59,999),
		"new Date( 1999,11,31,23,59,59,999)",
		[TIME_2000-PST_ADJUST-1,2000,0,1,6,7,59,59,999,1999,11,31,5,23,59,59,999] );

addNewTestCase( new Date( 2000,0,1,0,0,0,0),
		"new Date( 2000,0,1,0,0,0,0)",
		[TIME_2000-PST_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

addNewTestCase( new Date( 2000,0,1,0,0,0,1),
		"new Date( 2000,0,1,0,0,0,1)",
		[TIME_2000-PST_ADJUST+1,2000,0,1,6,8,0,0,1,2000,0,1,6,0,0,0,1] );



addNewTestCase( new Date(2000,1,28,16,0,0,0),
		"new Date(2000,1,28,16,0,0,0)",
		[UTC_FEB_29_2000,2000,1,29,2,0,0,0,0,2000,1,28,1,16,0,0,0] );

addNewTestCase( new Date(2000,1,29,0,0,0,0),
		"new Date(2000,1,29,0,0,0,0)",
		[UTC_FEB_29_2000-PST_ADJUST,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );

addNewTestCase( new Date(2000,1,28,24,0,0,0),
		"new Date(2000,1,28,24,0,0,0)",
		[UTC_FEB_29_2000-PST_ADJUST,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );



addNewTestCase( new Date(1899,11,31,16,0,0,0),
		"new Date(1899,11,31,16,0,0,0)",
		[TIME_1900,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );

addNewTestCase( new Date(1899,11,31,15,59,59,999),
		"new Date(1899,11,31,15,59,59,999)",
		[TIME_1900-1,1899,11,31,0,23,59,59,999,1899,11,31,0,15,59,59,999] );

addNewTestCase( new Date(1899,11,31,23,59,59,999),
		"new Date(1899,11,31,23,59,59,999)",
		[TIME_1900-PST_ADJUST-1,1900,0,1,1,7,59,59,999,1899,11,31,0,23,59,59,999] );

addNewTestCase( new Date(1900,0,1,0,0,0,0),
		"new Date(1900,0,1,0,0,0,0)",
		[TIME_1900-PST_ADJUST,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );

addNewTestCase( new Date(1900,0,1,0,0,0,1),
		"new Date(1900,0,1,0,0,0,1)",
		[TIME_1900-PST_ADJUST+1,1900,0,1,1,8,0,0,1,1900,0,1,1,0,0,0,1] );



addNewTestCase( new Date(2005,0,1,0,0,0,0),
		"new Date(2005,0,1,0,0,0,0)",
		[UTC_JAN_1_2005-PST_ADJUST,2005,0,1,6,8,0,0,0,2005,0,1,6,0,0,0,0] );

addNewTestCase( new Date(2004,11,31,16,0,0,0),
		"new Date(2004,11,31,16,0,0,0)",
		[UTC_JAN_1_2005,2005,0,1,6,0,0,0,0,2004,11,31,5,16,0,0,0] );

test();


function addNewTestCase( DateCase, DateString, ResultArray ) {
  
  adjustResultArray(ResultArray);

  new TestCase( SECTION, DateString+".getTime()", ResultArray[TIME],       DateCase.getTime() );
  new TestCase( SECTION, DateString+".valueOf()", ResultArray[TIME],       DateCase.valueOf() );

  new TestCase( SECTION, DateString+".getUTCFullYear()",      ResultArray[UTC_YEAR],  DateCase.getUTCFullYear() );
  new TestCase( SECTION, DateString+".getUTCMonth()",         ResultArray[UTC_MONTH],  DateCase.getUTCMonth() );
  new TestCase( SECTION, DateString+".getUTCDate()",          ResultArray[UTC_DATE],   DateCase.getUTCDate() );
  new TestCase( SECTION, DateString+".getUTCDay()",           ResultArray[UTC_DAY],    DateCase.getUTCDay() );
  new TestCase( SECTION, DateString+".getUTCHours()",         ResultArray[UTC_HOURS],  DateCase.getUTCHours() );
  new TestCase( SECTION, DateString+".getUTCMinutes()",       ResultArray[UTC_MINUTES],DateCase.getUTCMinutes() );
  new TestCase( SECTION, DateString+".getUTCSeconds()",       ResultArray[UTC_SECONDS],DateCase.getUTCSeconds() );
  new TestCase( SECTION, DateString+".getUTCMilliseconds()",  ResultArray[UTC_MS],     DateCase.getUTCMilliseconds() );

  new TestCase( SECTION, DateString+".getFullYear()",         ResultArray[YEAR],       DateCase.getFullYear() );
  new TestCase( SECTION, DateString+".getMonth()",            ResultArray[MONTH],      DateCase.getMonth() );
  new TestCase( SECTION, DateString+".getDate()",             ResultArray[DATE],       DateCase.getDate() );
  new TestCase( SECTION, DateString+".getDay()",              ResultArray[DAY],        DateCase.getDay() );
  new TestCase( SECTION, DateString+".getHours()",            ResultArray[HOURS],      DateCase.getHours() );
  new TestCase( SECTION, DateString+".getMinutes()",          ResultArray[MINUTES],    DateCase.getMinutes() );
  new TestCase( SECTION, DateString+".getSeconds()",          ResultArray[SECONDS],    DateCase.getSeconds() );
  new TestCase( SECTION, DateString+".getMilliseconds()",     ResultArray[MS],         DateCase.getMilliseconds() );
}

