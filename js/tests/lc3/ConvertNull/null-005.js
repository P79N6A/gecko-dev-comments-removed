





































gTestfile = 'null-005.js';








var SECTION = "Preferred argument conversion:  null";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001;




new TestCase(
  "Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001[\"staticAmbiguous(java.lang.Object)\"](null)",
  "STATIC_OBJECT",
  Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001["staticAmbiguous(java.lang.Object)"](null) +"");

new TestCase(
  "Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001[\"staticAmbiguous(java.lang.Boolean)\"](null)",
  "STATIC_BOOLEAN_OBJECT",
  Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001["staticAmbiguous(java.lang.Boolean)"](null) +"");

new TestCase(
  "Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001[\"staticAmbiguous(java.lang.String)\"](null)",
  "STATIC_STRING",
  Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001["staticAmbiguous(java.lang.String)"](null) +"");


test();
