





































gTestfile = 'string-002.js';







var SECTION = "Preferred argument conversion: string";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.string.String_002;

var string = "JavaScript Test String";

new TestCase(
  "TEST_CLASS.ambiguous(string)",
  TEST_CLASS.expect(),
  TEST_CLASS.ambiguous(string) );

test();
