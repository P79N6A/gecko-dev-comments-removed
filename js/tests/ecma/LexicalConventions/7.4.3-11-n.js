





































gTestfile = '7.4.3-11-n.js';



















var SECTION = "7.4.3-11-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var finally = true";
EXPECTED = "error";

new TestCase( SECTION,  "var finally = true",     "error",    eval("var finally = true") );

test();
