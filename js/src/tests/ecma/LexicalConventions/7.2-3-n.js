





































gTestfile = '7.2-3-n.js';





















var SECTION = "7.2-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Line Terminators";

writeHeaderToLog( SECTION + " "+ TITLE);


DESCRIPTION = "\r\nb";
EXPECTED = "error"

  new TestCase( SECTION,    "<cr>a",     "error",     eval("\r\nb"));

test();
