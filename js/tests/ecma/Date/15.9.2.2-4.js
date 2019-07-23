





































gTestfile = '15.9.2.2-4.js';














var VERSION = 9706;
startTest();
var SECTION = "15.9.2.2";
var TOLERANCE = 100;
var TITLE = "The Date Constructor Called as a Function";

writeHeaderToLog(SECTION+" "+TITLE );



new TestCase( SECTION, "Date( 2000,1,29,0,0,0)",        (new Date()).toString(),    Date(2000,1,29,0,0,0));
new TestCase( SECTION, "Date( 2000,1,28,23,59,59)",     (new Date()).toString(),    Date( 2000,1,28,23,59,59));
new TestCase( SECTION, "Date( 2000,1,27,16,0,0)",       (new Date()).toString(),    Date(2000,1,27,16,0,0));

test();
