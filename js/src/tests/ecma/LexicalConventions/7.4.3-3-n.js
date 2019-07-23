





































gTestfile = '7.4.3-3-n.js';



















var SECTION = "7.4.3-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var export = true";
EXPECTED = "error";

new TestCase( SECTION,  "var export = true",     "error",    eval("var export = true") );

test();
