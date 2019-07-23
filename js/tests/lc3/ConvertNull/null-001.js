











































var SECTION = "Preferred argument conversion:  null";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new
Packages.com.netscape.javascript.qa.lc3.jsnull.Null_001;

new TestCase(
    "TEST_CLASS[\"ambiguous(java.lang.Object)\"](null) +''",
    "OBJECT",
    TEST_CLASS["ambiguous(java.lang.Object)"](null) +'' );

new TestCase(
    "TEST_CLASS[\"ambiguous(java.lang.String)\"](null) +''",
    "STRING",
    TEST_CLASS["ambiguous(java.lang.String)"](null) +'' );

test();
