





































gTestfile = 'ToByte-001.js';








var SECTION = "Preferred argument conversion:  JavaScript Object to short";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_010;

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}

function MyFunction() {
  return;
}
MyFunction.valueOf = new Function( "return -128" );


new TestCase(
  "TEST_CLASS.ambiguous( new String() ) +''",
  "BYTE",
  TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Boolean() ) +''",
  "BYTE",
  TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Number() ) +''",
  "BYTE",
  TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Date(99) ) +''",
  "BYTE",
  TEST_CLASS.ambiguous( new Date(99) )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new MyObject(127) ) +''",
  "BYTE",
  TEST_CLASS.ambiguous( new MyObject(127) )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( MyFunction ) +''",
  "BYTE",
  TEST_CLASS.ambiguous( MyFunction )+'' );

test();


