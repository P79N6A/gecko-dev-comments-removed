





































gTestfile = '7.4.3-12-n.js';



















var SECTION = "7.4.3-12-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var throw = true";
EXPECTED = "error";

new TestCase( SECTION,  "var throw = true",     "error",    eval("var throw = true") );

test();
