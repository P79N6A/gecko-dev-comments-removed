





































gTestfile = '7.4.3-6-n.js';



















var SECTION = "7.4.3-6-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var default = true";
EXPECTED = "error";

new TestCase( SECTION,  "var default = true",     "error",    eval("var default = true") );

test();
