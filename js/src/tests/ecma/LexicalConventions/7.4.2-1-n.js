





































gTestfile = '7.4.2-1-n.js';

























var SECTION = "7.4.2-1-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var break = true";
EXPECTED = "error";

new TestCase( SECTION,  "var break = true",     "error",    eval("var break = true") );

test();
