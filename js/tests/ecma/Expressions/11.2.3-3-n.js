





































gTestfile = '11.2.3-3-n.js';




































var SECTION = "11.2.3-3-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function Calls";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "(void 0).valueOf()";
EXPECTED = "error";

new TestCase( SECTION,
              "(void 0).valueOf()",
              "error",
              eval("(void 0).valueOf()") );
test();

