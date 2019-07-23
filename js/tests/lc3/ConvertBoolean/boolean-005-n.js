









































var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new
Packages.com.netscape.javascript.qa.lc3.bool.Boolean_005;

DESCRIPTION = "TEST_CLASS.ambiguous( true )";
EXPECTED = "error";

new TestCase(
    "TEST_CLASS.ambiguous( true )",
    "error",
    TEST_CLASS.ambiguous(true) );

test();
