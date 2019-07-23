





































gTestfile = '7.4.2-7-n.js';

























var SECTION = "7.4.2-7";
var VERSION = "ECMA_1";
startTest();
writeHeaderToLog( SECTION + " Keywords");

DESCRIPTION = "var return = true";
EXPECTED = "error";

new TestCase( SECTION,  "var return = true",     "error",    eval("var return = true") );

test();
