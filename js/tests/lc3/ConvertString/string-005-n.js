










































var SECTION = "Preferred argument conversion: string";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new
Packages.com.netscape.javascript.qa.lc3.string.String_004;

var string = "255";

DESCRIPTION = "TEST_CLASS.ambiguous(string)";
EXPECTED = "error";

testcases[testcases.length] = new TestCase(
    "TEST_CLASS.ambiguous(string)",
    "error",
    TEST_CLASS.ambiguous(string) );

test();
