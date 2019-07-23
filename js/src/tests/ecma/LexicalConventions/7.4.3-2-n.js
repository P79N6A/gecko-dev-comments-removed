





































gTestfile = '7.4.3-2-n.js';



















var SECTION = "7.4.3-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var debugger = true";
EXPECTED = "error";

new TestCase( SECTION,  "var debugger = true",     "error",    eval("var debugger = true") );

test();
