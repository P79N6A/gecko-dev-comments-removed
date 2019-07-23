





































gTestfile = '11.13.js';





























var SECTION = "11.12";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Conditional operator( ? : )");

new TestCase( SECTION,    "true ? 'PASSED' : 'FAILED'",     "PASSED",       (true?"PASSED":"FAILED"));
new TestCase( SECTION,    "false ? 'FAILED' : 'PASSED'",     "PASSED",      (false?"FAILED":"PASSED"));

new TestCase( SECTION,    "1 ? 'PASSED' : 'FAILED'",     "PASSED",          (true?"PASSED":"FAILED"));
new TestCase( SECTION,    "0 ? 'FAILED' : 'PASSED'",     "PASSED",          (false?"FAILED":"PASSED"));
new TestCase( SECTION,    "-1 ? 'PASSED' : 'FAILED'",     "PASSED",          (true?"PASSED":"FAILED"));

new TestCase( SECTION,    "NaN ? 'FAILED' : 'PASSED'",     "PASSED",          (Number.NaN?"FAILED":"PASSED"));

new TestCase( SECTION,    "var VAR = true ? , : 'FAILED'", "PASSED",           (VAR = true ? "PASSED" : "FAILED") );

test();
