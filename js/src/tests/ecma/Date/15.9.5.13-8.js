





































gTestfile = '15.9.5.13-8.js';














var SECTION = "15.9.5.13";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.getUTCDay()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
              "(new Date(NaN)).getUTCDay()",
              NaN,
              (new Date(NaN)).getUTCDay() );

new TestCase( SECTION,
              "Date.prototype.getUTCDay.length",
              0,
              Date.prototype.getUTCDay.length );

test();
