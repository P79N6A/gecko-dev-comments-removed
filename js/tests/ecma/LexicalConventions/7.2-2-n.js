





































gTestfile = '7.2-2-n.js';





















var SECTION = "7.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Line Terminators";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "\r\r\r\nb";
EXPECTED = "error"

  new TestCase( SECTION,   DESCRIPTION,     "error",     eval("\r\r\r\nb"));

test();

