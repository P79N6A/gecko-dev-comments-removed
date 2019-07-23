





































gTestfile = '7.5-10-n.js';












var SECTION = "7.5-9-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var 123=\"hi\"";
EXPECTED = "error";

new TestCase( SECTION,    "var 123=\"hi\"",   "error",    eval("123 = \"hi\"; array[item] = 123;") );

test();
