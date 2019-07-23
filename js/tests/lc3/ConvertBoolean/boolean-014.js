





































gTestfile = 'boolean-014.js';








var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.bool.Boolean_001;



new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Boolean)\"](true)",
  TEST_CLASS.BOOLEAN_OBJECT,
  TEST_CLASS["ambiguous(java.lang.Boolean)"](true) );

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Boolean)\"](false)",
  TEST_CLASS.BOOLEAN_OBJECT,
  TEST_CLASS["ambiguous(java.lang.Boolean)"](false) );



new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Object)\"](true)",
  TEST_CLASS.OBJECT,
  TEST_CLASS["ambiguous(java.lang.Object)"](true) );

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Boolean)\"](false)",
  TEST_CLASS.OBJECT,
  TEST_CLASS["ambiguous(java.lang.Object)"](false) );



new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.String)\"](true)",
  TEST_CLASS.STRING,
  TEST_CLASS["ambiguous(java.lang.String)"](true) );

new TestCase(
  "TEST_CLASS[\"ambiguous(java.lang.Boolean)\"](false)",
  TEST_CLASS.STRING,
  TEST_CLASS["ambiguous(java.lang.String)"](false) );

test();
