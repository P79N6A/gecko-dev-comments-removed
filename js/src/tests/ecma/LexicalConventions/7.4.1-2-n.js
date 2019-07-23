





































gTestfile = '7.4.1-2-n.js';



















var SECTION = "7.4.1-2-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Keywords";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "var true = false";
EXPECTED = "error";

new TestCase( SECTION,  "var true = false",     "error",    eval("var true = false") );

test();
