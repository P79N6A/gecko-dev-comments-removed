



















































var SECTION = "15.9.5.22";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getTimezoneOffset()";

writeHeaderToLog( SECTION + " "+ TITLE);

addTestCase( TIME_1970 );

test();

function addTestCase( t ) {
  for ( m = 0; m <= 1000; m+=100 ) {
    t++;
    new TestCase( SECTION,
		  "(new Date("+t+")).getTimezoneOffset()",
		  (t - LocalTime(t)) / msPerMinute,
		  (new Date(t)).getTimezoneOffset() );
  }
}
