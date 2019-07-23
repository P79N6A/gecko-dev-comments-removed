





































gTestfile = '7.5-5-n.js';












var SECTION = "7.5-5-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var 3abc";
EXPECTED = "error";

new TestCase( SECTION,    "var 3abc",   "error",    eval("var 3abc") );

test();
