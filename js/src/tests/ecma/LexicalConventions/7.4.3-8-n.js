





































gTestfile = '7.4.3-8-n.js';



















var SECTION = "7.4.3-9-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var switch = true";
EXPECTED = "error";

new TestCase( SECTION,  "var switch = true",     "error",    eval("var switch = true") );

test();
