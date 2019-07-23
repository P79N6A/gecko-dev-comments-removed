





































gTestfile = '15.9.5.30-1.js';


























var SECTION = "15.9.5.30-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Date.prototype.setHours( hour [, min, sec, ms] )");

addNewTestCase( 0,0,0,0,void 0,
		"TDATE = new Date(0);(TDATE).setHours(0);TDATE" );

addNewTestCase( 28800000, 23, 59, 999,void 0,
		"TDATE = new Date(28800000);(TDATE).setHours(23,59,999);TDATE" );

addNewTestCase( 28800000, 999, 999, void 0, void 0,
		"TDATE = new Date(28800000);(TDATE).setHours(999,999);TDATE" );

addNewTestCase( 28800000,999,0, void 0, void 0,
		"TDATE = new Date(28800000);(TDATE).setHours(999);TDATE" );

addNewTestCase( 28800000,-8, void 0, void 0, void 0,
		"TDATE = new Date(28800000);(TDATE).setHours(-8);TDATE" );

addNewTestCase( 946684800000,8760, void 0, void 0, void 0,
                "TDATE = new Date(946684800000);(TDATE).setHours(8760);TDATE" );

addNewTestCase( TIME_2000 - msPerDay, 23, 59, 59, 999,
		"d = new Date( " + (TIME_2000-msPerDay) +"); d.setHours(23,59,59,999)" );

addNewTestCase( TIME_2000 - msPerDay, 23, 59, 59, 1000,
		"d = new Date( " + (TIME_2000-msPerDay) +"); d.setHours(23,59,59,1000)" );

test();

function addNewTestCase( time, hours, min, sec, ms, DateString) {
  var UTCDate =   UTCDateFromTime( SetHours( time, hours, min, sec, ms ));
  var LocalDate = LocalDateFromTime( SetHours( time, hours, min, sec, ms ));

  var DateCase = new Date( time );

  if ( min == void 0 ) {
    DateCase.setHours( hours );
  } else {
    if ( sec == void 0 ) {
      DateCase.setHours( hours, min );
    } else {
      if ( ms == void 0 ) {
	DateCase.setHours( hours, min, sec );
      } else {
	DateCase.setHours( hours, min, sec, ms );
      }
    }
  }


  new TestCase( SECTION, DateString+".getTime()",             UTCDate.value,       DateCase.getTime() );
  new TestCase( SECTION, DateString+".valueOf()",             UTCDate.value,       DateCase.valueOf() );

  new TestCase( SECTION, DateString+".getUTCFullYear()",      UTCDate.year,    DateCase.getUTCFullYear() );
  new TestCase( SECTION, DateString+".getUTCMonth()",         UTCDate.month,  DateCase.getUTCMonth() );
  new TestCase( SECTION, DateString+".getUTCDate()",          UTCDate.date,   DateCase.getUTCDate() );
  new TestCase( SECTION, DateString+".getUTCDay()",           UTCDate.day,    DateCase.getUTCDay() );
  new TestCase( SECTION, DateString+".getUTCHours()",         UTCDate.hours,  DateCase.getUTCHours() );
  new TestCase( SECTION, DateString+".getUTCMinutes()",       UTCDate.minutes,DateCase.getUTCMinutes() );
  new TestCase( SECTION, DateString+".getUTCSeconds()",       UTCDate.seconds,DateCase.getUTCSeconds() );
  new TestCase( SECTION, DateString+".getUTCMilliseconds()",  UTCDate.ms,     DateCase.getUTCMilliseconds() );

  new TestCase( SECTION, DateString+".getFullYear()",         LocalDate.year,       DateCase.getFullYear() );
  new TestCase( SECTION, DateString+".getMonth()",            LocalDate.month,      DateCase.getMonth() );
  new TestCase( SECTION, DateString+".getDate()",             LocalDate.date,       DateCase.getDate() );
  new TestCase( SECTION, DateString+".getDay()",              LocalDate.day,        DateCase.getDay() );
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

  d.day = WeekDay( t );
  d.time = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
  d.value = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );

  return (d);
}
function SetHours( t, hour, min, sec, ms ) {
  var TIME = LocalTime(t);
  var HOUR = Number(hour);
  var MIN =  ( min == void 0) ? MinFromTime(TIME) : Number(min);
  var SEC  = ( sec == void 0) ? SecFromTime(TIME) : Number(sec);
  var MS   = ( ms == void 0 ) ? msFromTime(TIME)  : Number(ms);
  var RESULT6 = MakeTime( HOUR,
			  MIN,
			  SEC,
			  MS );
  var UTC_TIME = UTC(  MakeDate(Day(TIME), RESULT6) );
  return ( TimeClip(UTC_TIME) );
}
