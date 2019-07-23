





































gTestfile = '15.9.5.8.js';














var SECTION = "15.9.5.8";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getMonth()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_NOW );
addTestCase( TIME_0000 );
addTestCase( TIME_1970 );
addTestCase( TIME_1900 );
addTestCase( TIME_2000 );
addTestCase( UTC_FEB_29_2000 );
addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
              "(new Date(NaN)).getMonth()",
              NaN,
              (new Date(NaN)).getMonth() );

new TestCase( SECTION,
              "Date.prototype.getMonth.length",
              0,
              Date.prototype.getMonth.length );
test();

function addTestCase( t ) {
  var leap = InLeapYear(t);

  for ( var m = 0; m < 12; m++ ) {

    t += TimeInMonth(m, leap);

    new TestCase( SECTION,
                  "(new Date("+t+")).getMonth()",
                  MonthFromTime(LocalTime(t)),
                  (new Date(t)).getMonth() );

    new TestCase( SECTION,
                  "(new Date("+(t+1)+")).getMonth()",
                  MonthFromTime(LocalTime(t+1)),
                  (new Date(t+1)).getMonth() );

    new TestCase( SECTION,
                  "(new Date("+(t-1)+")).getMonth()",
                  MonthFromTime(LocalTime(t-1)),
                  (new Date(t-1)).getMonth() );

    new TestCase( SECTION,
                  "(new Date("+(t-TZ_ADJUST)+")).getMonth()",
                  MonthFromTime(LocalTime(t-TZ_ADJUST)),
                  (new Date(t-TZ_ADJUST)).getMonth() );

    new TestCase( SECTION,
                  "(new Date("+(t+TZ_ADJUST)+")).getMonth()",
                  MonthFromTime(LocalTime(t+TZ_ADJUST)),
                  (new Date(t+TZ_ADJUST)).getMonth() );

  }
}
