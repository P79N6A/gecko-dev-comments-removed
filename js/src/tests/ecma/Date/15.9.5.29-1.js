





































gTestfile = '15.9.5.29-1.js';

























var SECTION = "15.9.5.29-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Date.prototype.setUTCMinutes( min [, sec, ms] )");

addNewTestCase( 0, 0, void 0, void 0,
		"TDATE = new Date(0);(TDATE).setUTCMinutes(0);TDATE",
		UTCDateFromTime(SetUTCMinutes(0,0,0,0)),
		LocalDateFromTime(SetUTCMinutes(0,0,0,0)) );

addNewTestCase( 28800000, 59, 59, void 0,
		"TDATE = new Date(28800000);(TDATE).setUTCMinutes(59,59);TDATE",
		UTCDateFromTime(SetUTCMinutes(28800000,59,59)),
		LocalDateFromTime(SetUTCMinutes(28800000,59,59)) );

addNewTestCase( 28800000, 59, 59, 999,
		"TDATE = new Date(28800000);(TDATE).setUTCMinutes(59,59,999);TDATE",
		UTCDateFromTime(SetUTCMinutes(28800000,59,59,999)),
		LocalDateFromTime(SetUTCMinutes(28800000,59,59,999)) );

addNewTestCase( 28800000, 59, void 0, void 0,
		"TDATE = new Date(28800000);(TDATE).setUTCMinutes(59);TDATE",
		UTCDateFromTime(SetUTCMinutes(28800000,59)),
		LocalDateFromTime(SetUTCMinutes(28800000,59)) );

addNewTestCase( 28800000, -480, 0, 0,
		"TDATE = new Date(28800000);(TDATE).setUTCMinutes(-480);TDATE",
		UTCDateFromTime(SetUTCMinutes(28800000,-480)),
		LocalDateFromTime(SetUTCMinutes(28800000,-480)) );

addNewTestCase( 946684800000, 1234567, void 0, void 0,
		"TDATE = new Date(946684800000);(TDATE).setUTCMinutes(1234567);TDATE",
		UTCDateFromTime(SetUTCMinutes(946684800000,1234567)),
		LocalDateFromTime(SetUTCMinutes(946684800000,1234567)) );

addNewTestCase( -2208988800000, 59, 999, void 0,
		"TDATE = new Date(-2208988800000);(TDATE).setUTCMinutes(59,999);TDATE",
		UTCDateFromTime(SetUTCMinutes(-2208988800000,59,999)),
		LocalDateFromTime(SetUTCMinutes(-2208988800000,59,999)) );

test();

function addNewTestCase( time, min, sec, ms, DateString, UTCDate, LocalDate) {
  var DateCase = new Date( time );

  if ( sec == void 0 ) {
    DateCase.setUTCMinutes( min );
  } else {
    if ( ms == void 0 ) {
      DateCase.setUTCMinutes( min, sec );
    } else {
      DateCase.setUTCMinutes( min, sec, ms );
    }
  }

  new TestCase( SECTION, DateString+".getTime()",             UTCDate.value,       DateCase.getTime() );
  new TestCase( SECTION, DateString+".valueOf()",             UTCDate.value,       DateCase.valueOf() );

  new TestCase( SECTION, DateString+".getUTCFullYear()",      UTCDate.year,    DateCase.getUTCFullYear() );
  new TestCase( SECTION, DateString+".getUTCMonth()",         UTCDate.month,  DateCase.getUTCMonth() );
  new TestCase( SECTION, DateString+".getUTCDate()",          UTCDate.date,   DateCase.getUTCDate() );

  new TestCase( SECTION, DateString+".getUTCHours()",         UTCDate.hours,  DateCase.getUTCHours() );
  new TestCase( SECTION, DateString+".getUTCMinutes()",       UTCDate.minutes,DateCase.getUTCMinutes() );
  new TestCase( SECTION, DateString+".getUTCSeconds()",       UTCDate.seconds,DateCase.getUTCSeconds() );
  new TestCase( SECTION, DateString+".getUTCMilliseconds()",  UTCDate.ms,     DateCase.getUTCMilliseconds() );

  new TestCase( SECTION, DateString+".getFullYear()",         LocalDate.year,       DateCase.getFullYear() );
  new TestCase( SECTION, DateString+".getMonth()",            LocalDate.month,      DateCase.getMonth() );
  new TestCase( SECTION, DateString+".getDate()",             LocalDate.date,       DateCase.getDate() );

  new TestCase( SECTION, DateString+".getHours()",            LocalDate.hours,      DateCase.getHours() );
  new TestCase( SECTION, DateString+".getMinutes()",          LocalDate.minutes,    DateCase.getMinutes() );
  new TestCase( SECTION, DateString+".getSeconds()",          LocalDate.seconds,    DateCase.getSeconds() );
  new TestCase( SECTION, DateString+".getMilliseconds()",     LocalDate.ms,         DateCase.getMilliseconds() );

  DateCase.toString = Object.prototype.toString;

  new TestCase( SECTION,
		DateString+".toString=Object.prototype.toString;"+DateString+".toString()",
		"[object Date]",
		DateCase.toString() );
}
function MyDate() {
  this.year = 0;
  this.month = 0;
  this.date = 0;
  this.hours = 0;
  this.minutes = 0;
  this.seconds = 0;
  this.ms = 0;
}
function LocalDateFromTime(t) {
  t = LocalTime(t);
  return ( MyDateFromTime(t) );
}
function UTCDateFromTime(t) {
  return ( MyDateFromTime(t) );
}
function MyDateFromTime( t ) {
  var d = new MyDate();
  d.year = YearFromTime(t);
  d.month = MonthFromTime(t);
  d.date = DateFromTime(t);
  d.hours = HourFromTime(t);
  d.minutes = MinFromTime(t);
  d.seconds = SecFromTime(t);
  d.ms = msFromTime(t);

  d.time = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
  d.value = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );
  d.day = WeekDay( d.value );

  return (d);
}
function SetUTCMinutes( t, min, sec, ms ) {
  var TIME = t;
  var MIN =  Number(min);
  var SEC  = ( sec == void 0) ? SecFromTime(TIME) : Number(sec);
  var MS   = ( ms == void 0 ) ? msFromTime(TIME)  : Number(ms);
  var RESULT5 = MakeTime( HourFromTime( TIME ),
			  MIN,
			  SEC,
			  MS );
  return ( TimeClip(MakeDate(Day(TIME),RESULT5)) );
}
