





































gTestfile = '7.4.2-14-n.js';

























var SECTION = "7.4.1-14-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var in = true";
EXPECTED = "error";

new TestCase( SECTION,  "var in = true",     "error",    eval("var in = true") );

test();
