





































gTestfile = '7.2-5-n.js';





















var SECTION = "7.2-5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Line Terminators";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION =
  EXPECTED = "error";

new TestCase( SECTION,    "\rb",     "error",    eval("\rb"));
test();
