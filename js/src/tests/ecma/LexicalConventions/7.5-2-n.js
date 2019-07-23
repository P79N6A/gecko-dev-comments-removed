





































gTestfile = '7.5-2-n.js';












var SECTION = "7.5-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var 0abc";
EXPECTED = "error";

new TestCase( SECTION,    "var 0abc",   "error",    eval("var 0abc") );

test();
