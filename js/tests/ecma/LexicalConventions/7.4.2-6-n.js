





































gTestfile = '7.4.2-6-n.js';

























var SECTION = "7.4.2-6-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var function = true";
EXPECTED = "error";

new TestCase( SECTION,  "var function = true",     "error",    eval("var function = true") );

test();
