





































gTestfile = '7.5-3-n.js';












var SECTION = "7.5-3-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var 1abc";
EXPECTED = "error";

new TestCase( SECTION,    "var 1abc",   "error",    eval("var 1abc") );

test();
