





































gTestfile = '7.4.1-1-n.js';



















var SECTION = "7.4.1-1-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var null = true";
EXPECTED = "error";

new TestCase( SECTION,  "var null = true",     "error",    eval("var null = true") );

test();
