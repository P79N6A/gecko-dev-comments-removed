





































gTestfile = '7.4.2-11-n.js';

























var SECTION = "7.4.1-11-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var this = true";
EXPECTED = "error";

new TestCase( SECTION,  "var this = true",     "error",    eval("var this = true") );

test();
