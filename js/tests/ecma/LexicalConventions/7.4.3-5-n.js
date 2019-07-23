





































gTestfile = '7.4.3-5-n.js';



















var SECTION = "7.4.3-5-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Future Reserved Words";

writeHeaderToLog( SECTION + " "+ TITLE);
   
DESCRIPTION = "var catch = true";
EXPECTED = "error";

new TestCase( SECTION,  "var catch = true",     "error",    eval("var catch = true") );

test();
