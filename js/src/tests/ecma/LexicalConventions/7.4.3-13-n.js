





































gTestfile = '7.4.3-13-n.js';



















var SECTION = "7.4.3-13-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var const = true";
EXPECTED = "error";

new TestCase( SECTION,  "var const = true",     "error",    eval("var const = true") );

test();
