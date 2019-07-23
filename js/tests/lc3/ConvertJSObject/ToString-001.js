











































var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_003;

new TestCase(
    "TEST_CLASS.ambiguous( new String() ) +''",
    "STRING",
    TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Boolean() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Number() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Date() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new Date() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Function() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new Function() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Array() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new Array() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( this ) +''",
    "STRING",
    TEST_CLASS.ambiguous( this )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new RegExp() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new RegExp() )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( Math ) +''",
    "STRING",
    TEST_CLASS.ambiguous( Math )+'' );

new TestCase(
    "TEST_CLASS.ambiguous( new Object() ) +''",
    "STRING",
    TEST_CLASS.ambiguous( new Object() )+'' );

test();
