





































gTestfile = 'number-010.js';







var SECTION = "Preferred argument conversion:  undefined";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.number.Number_010;

new TestCase(
  "TEST_CLASS.ambiguous(1)",
  TEST_CLASS.expect(),
  TEST_CLASS.ambiguous(1) );

test();
