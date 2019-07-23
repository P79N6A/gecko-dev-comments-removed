





































gTestfile = '7.4.2-9-n.js';

























var SECTION = "7.4.1-9-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var delete = true";
EXPECTED = "error";

new TestCase( SECTION,  "var delete = true",     "error",    eval("var delete = true") );

test();
