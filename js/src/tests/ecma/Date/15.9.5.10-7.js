


















var SECTION = "15.9.5.10";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getDate()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( UTC_JAN_1_2005 );

new TestCase( SECTION,
              "(new Date(NaN)).getDate()",
              NaN,
              (new Date(NaN)).getDate() );

new TestCase( SECTION,
              "Date.prototype.getDate.length",
              0,
              Date.prototype.getDate.length );
test();

function addTestCase( t ) {
  var start = TimeFromYear(YearFromTime(t));
  var stop  = TimeFromYear(YearFromTime(t) + 1);

  for (var d = start; d < stop; d += msPerDay)
  {
    new TestCase( SECTION,
                  "(new Date("+d+")).getDate()",
                  DateFromTime(LocalTime(d)),
                  (new Date(d)).getDate() );
  }
}
