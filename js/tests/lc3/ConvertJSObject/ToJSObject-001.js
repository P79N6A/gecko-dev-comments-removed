





































gTestfile = 'ToJSObject-001.js';








var SECTION = "Preferred argument conversion:  boolean";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new
  Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_001;

new TestCase(
  "TEST_CLASS.ambiguous( new String() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Boolean() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Number() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Date() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new Date() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Function() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new Function() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Array() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new Array() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( this ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( this )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new RegExp() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new RegExp() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( Math ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( Math )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Object() ) +''",
  "JSOBJECT",
  TEST_CLASS.ambiguous( new Object() )+'' );

test();
