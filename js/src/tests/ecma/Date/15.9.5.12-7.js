





































gTestfile = '15.9.5.12-7.js';















var SECTION = "15.9.5.12";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getDay()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( UTC_JAN_1_2005 );

test();

function addTestCase( t ) {
  var start = TimeFromYear(YearFromTime(t));
  var stop  = TimeFromYear(YearFromTime(t) + 1);

  for (var d = start; d < stop; d += msPerDay)
  {
    new TestCase( SECTION,
                  "(new Date("+d+")).getDay()",
                  WeekDay((LocalTime(d))),
                  (new Date(d)).getDay() );
  }
}
