





































gTestfile = '11.2.3-4-n.js';




































var SECTION = "11.2.3-4-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function Calls";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "null.valueOf()";
EXPECTED = "error";

new TestCase( SECTION,
              "null.valueOf()",
              "error",
              eval("null.valueOf()") );
test();

