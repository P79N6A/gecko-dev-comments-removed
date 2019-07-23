





































gTestfile = '7.4.3-10-n.js';



















var SECTION = "7.4.3-10-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var do = true";
EXPECTED = "error";

new TestCase( SECTION,  "var do = true",     "error",    eval("var do = true") );

test();
