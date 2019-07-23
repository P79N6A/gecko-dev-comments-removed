





































gTestfile = '7.5-4-n.js';












var SECTION = "7.5-4-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var 2abc";
EXPECTED = "error";

new TestCase( SECTION,    "var 2abc",   "error",    eval("var 2abc") );

test();
