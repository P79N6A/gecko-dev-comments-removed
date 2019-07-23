





































gTestfile = '7.4.1-3-n.js';



















var SECTION = "7.4.1-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

DESCRIPTION = "var false = true";
EXPECTED = "error";

new TestCase( SECTION,  "var false = true",     "error",    eval("var false = true") );

test();
