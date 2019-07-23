





































gTestfile = '11.2.2-7-n.js';












































var SECTION = "11.2.2-6-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var STRING = new String("hi");

DESCRIPTION = "var STRING = new String('hi'); var s = new STRING()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var STRING = new String('hi'); var s = new STRING()",
	      "error",
	      eval("s = new STRING()") );
test();

function TestFunction() {
  return arguments;
}
