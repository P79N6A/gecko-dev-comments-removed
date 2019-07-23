









































var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new
Packages.com.netscape.javascript.qa.lc3.bool.Boolean_004;

new TestCase(
    "TEST_CLASS.ambiguous( true )",
    TEST_CLASS.expect(),
    TEST_CLASS.ambiguous(true) );

new TestCase(
    "TEST_CLASS.ambiguous( false )",
    TEST_CLASS.expect(),
    TEST_CLASS.ambiguous( false ) );

test();
