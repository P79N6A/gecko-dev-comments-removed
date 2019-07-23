





































gTestfile = '7.4.2-16-n.js';

























var SECTION = "7.4.1-16-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var with = true";
EXPECTED = "error";

new TestCase( SECTION,  "var with = true",     "error",    eval("var with = true") );

test();
