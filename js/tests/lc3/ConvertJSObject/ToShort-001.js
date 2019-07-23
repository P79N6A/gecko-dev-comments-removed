











































var SECTION = "Preferred argument conversion:  JavaScript Object to short";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_008;

new TestCase(
    "TEST_CLASS.ambiguous( new String() ) +''",
    "SHORT",
    TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Boolean() ) +''",
    "SHORT",
    TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Number() ) +''",
    "SHORT",
    TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Date(999) ) +''",
    "SHORT",
    TEST_CLASS.ambiguous( new Date(999) )+'' );

test();


