





































gTestfile = '15.9.5.37-4.js';


























var SECTION = "15.9.5.37-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Date.prototype.setUTCFullYear(year [, mon [, date ]] )");



addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2005);TDATE",
		UTCDateFromTime(SetUTCFullYear(0,2005)),
		LocalDateFromTime(SetUTCFullYear(0,2005)) );

addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2004);TDATE",
		UTCDateFromTime(SetUTCFullYear(0,2004)),
		LocalDateFromTime(SetUTCFullYear(0,2004)) );

addNewTestCase( "TDATE = new Date(0); TDATE.setUTCFullYear(2006);TDATE",
		UTCDateFromTime(SetUTCFullYear(0,2006)),
		LocalDateFromTime(SetUTCFullYear(0,2006)) );

test();

function addNewTestCase( DateString, UTCDate, LocalDate) {
  DateCase = eval( DateString );




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

  d.time = MakeTime( d.hours, d.minutes, d.seconds, d.ms );
  d.value = TimeClip( MakeDate( MakeDay( d.year, d.month, d.date ), d.time ) );
  d.day = WeekDay( d.value );

  return (d);
}
function SetUTCFullYear( t, year, mon, date ) {
  var T = ( t != t ) ? 0 : t;
  var YEAR = Number(year);
  var MONTH = ( mon == void 0 ) ?     MonthFromTime(T) : Number( mon );
  var DATE  = ( date == void 0 ) ?    DateFromTime(T)  : Number( date );
  var DAY = MakeDay( YEAR, MONTH, DATE );

  return ( TimeClip(MakeDate(DAY, TimeWithinDay(T))) );
}
