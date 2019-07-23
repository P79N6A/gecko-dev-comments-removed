





































gTestfile = '7.2-4-n.js';





















var SECTION = "7.2-6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Line Terminators";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "\nb";
EXPECTED = "error";

new TestCase( SECTION,    "\nb",     "error",     eval("\nb"));

test();
