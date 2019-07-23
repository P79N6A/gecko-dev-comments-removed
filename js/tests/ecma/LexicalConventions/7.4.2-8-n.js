





































gTestfile = '7.4.2-8-n.js';

























var SECTION = "7.4.2-8";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Keywords");

DESCRIPTION = "var void = true";
EXPECTED = "error";

new TestCase( SECTION,  "var void = true",     "error",    eval("var void = true") );

test();
