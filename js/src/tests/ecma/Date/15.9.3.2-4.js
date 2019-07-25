








































































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


var SECTION = "15.9.3.1";
var TITLE =   "Date( year, month, date, hours, minutes, seconds )";

writeHeaderToLog( SECTION+" " +TITLE );

var PST_FEB_29_2000 = UTC_FEB_29_2000 + 8*msPerHour;


addNewTestCase( new Date(2000,1,28,16,0,0,0),
		"new Date(2000,1,28,16,0,0,0)",
		[UTC_FEB_29_2000,2000,1,29,2,0,0,0,0,2000,1,28,1,16,0,0,0,0] );

addNewTestCase( new Date(2000,1,29,0,0,0,0),
		"new Date(2000,1,29,0,0,0,0)",
		[PST_FEB_29_2000,2000,1,29,2,8,0,0,0,2000,1,29,2,0,0,0,0] );

addNewTestCase( new Date(2000,1,29,24,0,0,0),
		"new Date(2000,1,29,24,0,0,0)",
		[PST_FEB_29_2000+msPerDay,2000,2,1,3,8,0,0,0,2000,2,1,3,0,0,0,0] );

test();

function addNewTestCase( DateCase, DateString, ResultArray ) {
  
  adjustResultArray(ResultArray);

  new TestCase( SECTION, DateString+".getTime()", ResultArray[TIME],       DateCase.getTime() );
  new TestCase( SECTION, DateString+".valueOf()", ResultArray[TIME],       DateCase.valueOf() );

  new TestCase( SECTION, DateString+".getUTCFullYear()",      ResultArray[UTC_YEAR],   DateCase.getUTCFullYear() );
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
