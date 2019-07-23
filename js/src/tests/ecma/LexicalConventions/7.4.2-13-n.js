





































gTestfile = '7.4.2-13-n.js';

























var SECTION = "7.4.1-13-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var else = true";
EXPECTED = "error";

new TestCase( SECTION,  "var else = true",     "error",    eval("var else = true") );

test();
