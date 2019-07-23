





















































var SECTION = "15.9.5.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Date.prototype.toString.length",
	      0,
	      Date.prototype.toString.length );

var now = new Date();




new TestCase( SECTION,
	      "Math.abs(Date.parse(now.toString()) - now.valueOf()) < 1000",
	      true,
	      Math.abs(Date.parse(now.toString()) - now.valueOf()) < 1000 );

new TestCase( SECTION,
	      "typeof now.toString()",
	      "string",
	      typeof now.toString() );


TZ_ADJUST = TZ_DIFF * msPerHour;

new TestCase( SECTION,
	      "Date.parse( (new Date(0)).toString() )",
	      0,
	      Date.parse( (new Date(0)).toString() ) );

new TestCase( SECTION,
	      "Date.parse( (new Date("+TZ_ADJUST+")).toString() )",
	      TZ_ADJUST,
	      Date.parse( (new Date(TZ_ADJUST)).toString() ) );


new TestCase( SECTION,
	      "Date.parse( (new Date("+TIME_1900+")).toString() )",
	      TIME_1900,
	      Date.parse( (new Date(TIME_1900)).toString() ) );

new TestCase( SECTION,
	      "Date.parse( (new Date("+TIME_1900 -TZ_ADJUST+")).toString() )",
	      TIME_1900 -TZ_ADJUST,
	      Date.parse( (new Date(TIME_1900 -TZ_ADJUST)).toString() ) );


new TestCase( SECTION,
	      "Date.parse( (new Date("+TIME_2000+")).toString() )",
	      TIME_2000,
	      Date.parse( (new Date(TIME_2000)).toString() ) );

new TestCase( SECTION,
	      "Date.parse( (new Date("+TIME_2000 -TZ_ADJUST+")).toString() )",
	      TIME_2000 -TZ_ADJUST,
	      Date.parse( (new Date(TIME_2000 -TZ_ADJUST)).toString() ) );



var UTC_29_FEB_2000 = TIME_2000 + 31*msPerDay + 28*msPerDay;
new TestCase( SECTION,
	      "Date.parse( (new Date("+UTC_29_FEB_2000+")).toString() )",
	      UTC_29_FEB_2000,
	      Date.parse( (new Date(UTC_29_FEB_2000)).toString() ) );

new TestCase( SECTION,
	      "Date.parse( (new Date("+(UTC_29_FEB_2000-1000)+")).toString() )",
	      UTC_29_FEB_2000-1000,
	      Date.parse( (new Date(UTC_29_FEB_2000-1000)).toString() ) );


new TestCase( SECTION,
	      "Date.parse( (new Date("+(UTC_29_FEB_2000-TZ_ADJUST)+")).toString() )",
	      UTC_29_FEB_2000-TZ_ADJUST,
	      Date.parse( (new Date(UTC_29_FEB_2000-TZ_ADJUST)).toString() ) );


var UTC_1_JAN_2005 = TIME_2000 + TimeInYear(2000) + TimeInYear(2001) +
TimeInYear(2002) + TimeInYear(2003) + TimeInYear(2004);
new TestCase( SECTION,
	      "Date.parse( (new Date("+UTC_1_JAN_2005+")).toString() )",
	      UTC_1_JAN_2005,
	      Date.parse( (new Date(UTC_1_JAN_2005)).toString() ) );

new TestCase( SECTION,
	      "Date.parse( (new Date("+(UTC_1_JAN_2005-1000)+")).toString() )",
	      UTC_1_JAN_2005-1000,
	      Date.parse( (new Date(UTC_1_JAN_2005-1000)).toString() ) );

new TestCase( SECTION,
	      "Date.parse( (new Date("+(UTC_1_JAN_2005-TZ_ADJUST)+")).toString() )",
	      UTC_1_JAN_2005-TZ_ADJUST,
	      Date.parse( (new Date(UTC_1_JAN_2005-TZ_ADJUST)).toString() ) );

test();
