





































gTestfile = '15.9.3.8-3.js';































var VERSION = "ECMA_1";
startTest();
var SECTION = "15.9.3.8";
var TYPEOF  = "object";

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



var gTc= 0;
var TITLE = "Date constructor:  new Date( value )";
var SECTION = "15.9.3.8";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION +" " + TITLE );


var TZ_ADJUST =  -TZ_PST * msPerHour;




addNewTestCase( new Date(TIME_2000+TZ_ADJUST),
		"new Date(" +(TIME_2000+TZ_ADJUST)+")",
		[TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

addNewTestCase( new Date(TIME_2000),
		"new Date(" +TIME_2000+")",
		[TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );

addNewTestCase( new Date( (new Date(TIME_2000+TZ_ADJUST)).toString()),
		"new Date(\"" +(new Date(TIME_2000+TZ_ADJUST)).toString()+"\")",
		[TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

addNewTestCase( new Date((new Date(TIME_2000)).toString()),
		"new Date(\"" +(new Date(TIME_2000)).toString()+"\")",
		[TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );


addNewTestCase(  new Date( (new Date(TIME_2000+TZ_ADJUST)).toUTCString()),
		 "new Date(\"" +(new Date(TIME_2000+TZ_ADJUST)).toUTCString()+"\")",
		 [TIME_2000+TZ_ADJUST,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );

addNewTestCase( new Date( (new Date(TIME_2000)).toUTCString()),
		"new Date(\"" +(new Date(TIME_2000)).toUTCString()+"\")",
		[TIME_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );

test();

function addNewTestCase( DateCase, DateString, ResultArray ) {
  
  adjustResultArray(ResultArray, 'msMode');

  new TestCase( SECTION, DateString+".getTime()", ResultArray[TIME],       DateCase.getTime() );
  new TestCase( SECTION, DateString+".valueOf()", ResultArray[TIME],       DateCase.valueOf() );
  new TestCase( SECTION, DateString+".getUTCFullYear()",      ResultArray[UTC_YEAR], DateCase.getUTCFullYear() );
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
