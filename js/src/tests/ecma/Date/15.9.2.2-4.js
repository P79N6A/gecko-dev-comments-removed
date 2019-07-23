





































gTestfile = '15.9.2.2-4.js';














var VERSION = 9706;
startTest();
var SECTION = "15.9.2.2";
var TOLERANCE = 100;
var TITLE = "The Date Constructor Called as a Function";

writeHeaderToLog(SECTION+" "+TITLE );




var d1;
var d2;



d1 = new Date();
d2 = Date.parse(Date(2000,1,29,0,0,0));
new TestCase(SECTION, "Date(2000,1,29,0,0,0)", true, d2 - d1 <= 1000);

d1 = new Date();
d2 = Date.parse(Date(2000,1,28,23,59,59));
new TestCase(SECTION, "Date(2000,1,28,23,59,59)", true, d2 - d1 <= 1000);

d1 = new Date();
d2 = Date.parse(Date(2000,1,27,16,0,0));
new TestCase(SECTION, "Date(2000,1,27,16,0,0)", true, d2 - d1 <= 1000);

test();
