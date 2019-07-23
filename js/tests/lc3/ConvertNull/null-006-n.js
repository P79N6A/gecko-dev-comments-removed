











































var SECTION = "Preferred argument conversion:  null";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

TEST_CLASS = new
Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001;




DESCRIPTION = "Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001[\"staticAmbiguous(java.lang.Class)\"](null)";
EXPECTED = "error";

new TestCase(
    "Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001[\"staticAmbiguous(java.lang.Class)\"](null)",
    "error",
    Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001["staticAmbiguous(java.lang.Class)"](null) +"");

test();
