





































gTestfile = 'undefined-001-n.js';








var SECTION = "Preferred argument conversion:  undefined";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.undefined.Undefined_001;

DESCRIPTION = "TEST_CLASS.ambiguous( void 0 )";
EXPECTED = "error";

new TestCase(
  "TEST_CLASS.ambiguous( void 0 )",
  "error",
  TEST_CLASS.ambiguous(void 0) );

test();
