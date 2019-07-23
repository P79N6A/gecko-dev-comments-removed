











































var SECTION = "Preferred argument conversion: JavaScript object to double";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_004;

new TestCase(
    "TEST_CLASS.ambiguous( new String() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Boolean() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Number() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Date() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( new Date() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Function() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( new Function() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( this ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( this )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new RegExp() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( new RegExp() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( Math ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( Math )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Object() ) +''",
    "DOUBLE",
    TEST_CLASS.ambiguous( new Object() )+'' );

test();
