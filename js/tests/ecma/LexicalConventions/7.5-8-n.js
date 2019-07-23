





































gTestfile = '7.5-8-n.js';












var SECTION = "7.5-8-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var @0abc = 5; @0abc";
EXPECTED = "error";

new TestCase( SECTION,    "var @0abc = 5; @0abc",   "error",    eval("var @0abc = 5; @0abc") );

test();
