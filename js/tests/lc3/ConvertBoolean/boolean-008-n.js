





































gTestfile = 'boolean-008-n.js';








var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.bool.Boolean_004;

DESCRIPTION = "TEST_CLASS[\"ambiguous(short)\"](true)";
EXPECTED = "error";

new TestCase(
  "TEST_CLASS[\"ambiguous(short)\"](true)",
  "error",
  TEST_CLASS["ambiguous(short)"](true) );

test();
