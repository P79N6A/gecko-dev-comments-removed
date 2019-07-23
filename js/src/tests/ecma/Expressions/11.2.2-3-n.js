





































gTestfile = '11.2.2-3-n.js';












































var SECTION = "11.2.2-3-n.js";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The new operator";

writeHeaderToLog( SECTION + " "+ TITLE);

var DESCRIPTION = "NULL = null; var o = new NULL()";
var EXPECTED = "error";
var NULL = null;

new TestCase( SECTION,
	      "NULL = null; var o = new NULL()",
	      "error",
	      eval("o = new NULL()") );
test();

