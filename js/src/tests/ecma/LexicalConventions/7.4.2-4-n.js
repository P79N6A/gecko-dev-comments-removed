





































gTestfile = '7.4.2-4-n.js';

























var SECTION = "7.4.2-4-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var var = true";
EXPECTED = "error";

TestCase( SECTION,  "var var = true",     "error",    eval("var var = true") );

test();
