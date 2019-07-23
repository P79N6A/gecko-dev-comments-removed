





































gTestfile = '15.9.5.34-1.js';



















var SECTION = "15.9.5.34-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Date.prototype.setMonth(mon [, date ] )");

getFunctionCases();


d = new Date(0);
d.setMonth(1,1,1,1,1,1);

addNewTestCase(
  "TDATE = new Date(0); TDATE.setMonth(1,1,1,1,1,1); TDATE",
  UTCDateFromTime(SetMonth(0,1,1)),
  LocalDateFromTime(SetMonth(0,1,1)) );




addNewTestCase( "TDATE = new Date(TIME_NOW); (TDATE).setMonth(11,31); TDATE",
		UTCDateFromTime(SetMonth(TIME_NOW,11,31)),
		LocalDateFromTime(SetMonth(TIME_NOW,11,31)) );



addNewTestCase( "TDATE = new Date(0);(TDATE).setMonth(0,1);TDATE",
		UTCDateFromTime(SetMonth(0,0,1)),
		LocalDateFromTime(SetMonth(0,0,1)) );

addNewTestCase( "TDATE = new Date("+TIME_1900+"); "+
		"(TDATE).setMonth(11,31); TDATE",
		UTCDateFromTime( SetMonth(TIME_1900,11,31) ),
		LocalDateFromTime( SetMonth(TIME_1900,11,31) ) );

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

function getFunctionCases() {
  
  new TestCase(
    SECTION,
    "Date.prototype.setMonth.length",
    2,
    Date.prototype.setMonth.length );

  new TestCase(
    SECTION,
    "typeof Date.prototype.setMonth",
    "function",
    typeof Date.prototype.setMonth );

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
function SetMonth( t, mon, date ) {
  var TIME = LocalTime(t);
  var MONTH = Number( mon );
  var DATE = ( date == void 0 ) ? DateFromTime(TIME) : Number( date );
  var DAY = MakeDay( YearFromTime(TIME), MONTH, DATE );
  return ( TimeClip (UTC(MakeDate( DAY, TimeWithinDay(TIME) ))) );
}
