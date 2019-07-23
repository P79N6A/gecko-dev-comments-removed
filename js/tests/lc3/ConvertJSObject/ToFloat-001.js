





































gTestfile = 'ToFloat-001.js';








var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_005;

new TestCase(
  "TEST_CLASS.ambiguous( new String() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Boolean() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Number() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Date() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( new Date() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Function() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( new Function() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( this ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( this )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new RegExp() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( new RegExp() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( Math ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( Math )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Object() ) +''",
  "FLOAT",
  TEST_CLASS.ambiguous( new Object() )+'' );

test();
