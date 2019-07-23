





































gTestfile = '15.9.2.2-6.js';














var VERSION = 9706;
startTest();
var SECTION = "15.9.2.2";
var TOLERANCE = 100;
var TITLE = "The Date Constructor Called as a Function";

writeHeaderToLog(SECTION+" "+TITLE );


new TestCase( SECTION, "Date(2031,11,31,23,59,59)",     (new Date()).toString(),    Date(2031,11,31,23,59,59));
new TestCase( SECTION, "Date(2032,0,1,0,0,0)",          (new Date()).toString(),    Date(2032,0,1,0,0,0) );
new TestCase( SECTION, "Date(2032,0,1,0,0,1)",          (new Date()).toString(),    Date(2032,0,1,0,0,1) );
new TestCase( SECTION, "Date(2031,11,31,16,0,0,0)",     (new Date()).toString(),    Date(2031,11,31,16,0,0,0));
test();
