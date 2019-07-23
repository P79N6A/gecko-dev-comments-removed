





































gTestfile = '15.9.5.13-3.js';














var SECTION = "15.9.5.13";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCDay()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_1970 );

test();

function addTestCase( t ) {
  var start = TimeFromYear(YearFromTime(t));
  var stop  = TimeFromYear(YearFromTime(t) + 1);

  for (var d = start; d < stop; d += msPerDay)
  {
    new TestCase( SECTION,
                  "(new Date("+d+")).getUTCDay()",
                  WeekDay((d)),
                  (new Date(d)).getUTCDay() );
  }
}

