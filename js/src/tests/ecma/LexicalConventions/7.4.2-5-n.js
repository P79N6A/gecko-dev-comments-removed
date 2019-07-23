





































gTestfile = '7.4.2-5-n.js';

























var SECTION = "7.4.2-5-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var continue = true";
EXPECTED = "error";

new TestCase( SECTION,  "var continue = true",     "error",    eval("var continue = true") );

test();
