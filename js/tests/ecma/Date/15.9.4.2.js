





































gTestfile = '15.9.4.2.js';








































var VERSION = "ECMA_1";
startTest();
var SECTION = "15.9.4.2";
var TITLE   = "Date.parse()";

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
var TYPEOF  = "object";


writeHeaderToLog("15.9.4.2 Date.parse()" );



addNewTestCase( new Date(0),
		"new Date(0)",
		[0,1970,0,1,4,0,0,0,0,1969,11,31,3,16,0,0,0] );

addNewTestCase( new Date(-1),
		"new Date(-1)",
		[-1,1969,11,31,3,23,59,59,999,1969,11,31,3,15,59,59,999] );
addNewTestCase( new Date(28799999),
		"new Date(28799999)",
		[28799999,1970,0,1,4,7,59,59,999,1969,11,31,3,23,59,59,999] );
addNewTestCase( new Date(28800000),
		"new Date(28800000)",
		[28800000,1970,0,1,4,8,0,0,0,1970,0,1,4,0,0,0,0] );



addNewTestCase( new Date(946684799999),
		"new Date(946684799999)",
		[946684799999,1999,11,31,5,23,59,59,999,1999,11,31,5,15,59,59,999] );
addNewTestCase( new Date(946713599999),
		"new Date(946713599999)",
		[946713599999,2000,0,1,6,7,59,59,999,1999,11,31,5,23,59,59,999] );
addNewTestCase( new Date(946684800000),
		"new Date(946684800000)",
		[946684800000,2000,0,1,6,0,0,0,0,1999,11,31,5, 16,0,0,0] );
addNewTestCase( new Date(946713600000),
		"new Date(946713600000)",
		[946713600000,2000,0,1,6,8,0,0,0,2000,0,1,6,0,0,0,0] );



addNewTestCase( new Date(-2208988800000),
		"new Date(-2208988800000)",
		[-2208988800000,1900,0,1,1,0,0,0,0,1899,11,31,0,16,0,0,0] );

addNewTestCase( new Date(-2208988800001),
		"new Date(-2208988800001)",
		[-2208988800001,1899,11,31,0,23,59,59,999,1899,11,31,0,15,59,59,999] );

addNewTestCase( new Date(-2208960000001),
		"new Date(-2208960000001)",
		[-2208960000001,1900,0,1,1,7,59,59,0,1899,11,31,0,23,59,59,999] );
addNewTestCase( new Date(-2208960000000),
		"new Date(-2208960000000)",
		[-2208960000000,1900,0,1,1,8,0,0,0,1900,0,1,1,0,0,0,0] );
addNewTestCase( new Date(-2208959999999),
		"new Date(-2208959999999)",
		[-2208959999999,1900,0,1,1,8,0,0,1,1900,0,1,1,0,0,0,1] );



var PST_FEB_29_2000 = UTC_FEB_29_2000 + 8*msPerHour;

addNewTestCase( new Date(UTC_FEB_29_2000),
		"new Date(" + UTC_FEB_29_2000 +")",
		[UTC_FEB_29_2000,2000,0,1,6,0,0,0,0,1999,11,31,5,16,0,0,0] );
addNewTestCase( new Date(PST_FEB_29_2000),
		"new Date(" + PST_FEB_29_2000 +")",
		[PST_FEB_29_2000,2000,0,1,6,8.0,0,0,2000,0,1,6,0,0,0,0]);



var PST_JAN_1_2005 = UTC_JAN_1_2005 + 8*msPerHour;

addNewTestCase( new Date(UTC_JAN_1_2005),
		"new Date("+ UTC_JAN_1_2005 +")",
		[UTC_JAN_1_2005,2005,0,1,6,0,0,0,0,2004,11,31,5,16,0,0,0] );
addNewTestCase( new Date(PST_JAN_1_2005),
		"new Date("+ PST_JAN_1_2005 +")",
		[PST_JAN_1_2005,2005,0,1,6,8,0,0,0,2005,0,1,6,0,0,0,0] );


test();

function addNewTestCase( DateCase, DateString, ResultArray ) {
  DateCase = DateCase;

  new TestCase( SECTION, DateString+".getTime()", ResultArray[TIME],       DateCase.getTime() );
  new TestCase( SECTION, DateString+".valueOf()", ResultArray[TIME],       DateCase.valueOf() );
  new TestCase( SECTION, "Date.parse(" + DateCase.toString() +")",    Math.floor(ResultArray[TIME]/1000)*1000,  Date.parse(DateCase.toString()) );
  new TestCase( SECTION, "Date.parse(" + DateCase.toGMTString() +")", Math.floor(ResultArray[TIME]/1000)*1000,  Date.parse(DateCase.toGMTString()) );
}
