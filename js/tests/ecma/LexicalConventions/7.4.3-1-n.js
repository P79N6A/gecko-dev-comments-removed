





































gTestfile = '7.4.3-1-n.js';



















var SECTION = "7.4.3-1-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var case = true";
EXPECTED = "error";

new TestCase( SECTION,  "var case = true",     "error",    eval("var case = true") );

test();
