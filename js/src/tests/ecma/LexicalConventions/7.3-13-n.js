





































gTestfile = '7.3-13-n.js';











var SECTION = "7.3-13-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Comments";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "nested comment";
EXPECTED = "error";

var testcase = new TestCase( SECTION,
			     "nested comment",
			     "error",
			     eval("/*/*\"fail\";*/*/"));

test();
