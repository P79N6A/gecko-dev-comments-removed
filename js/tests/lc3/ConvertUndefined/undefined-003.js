





































gTestfile = 'undefined-003.js';








var SECTION = "Preferred argument conversion:  undefined";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.undefined.Undefined_001;

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Object)\"](void 0) +''",
  "OBJECT",
  TEST_CLASS["ambiguous(java.lang.Object)"](void 0) +'' );

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.String)\"](void 0) +''",
  "STRING",
  TEST_CLASS["ambiguous(java.lang.String)"](void 0) +'' );

test();
