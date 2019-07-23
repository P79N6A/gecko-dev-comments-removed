





































gTestfile = 'ToLong-001.js';








var SECTION = "Preferred argument conversion:  JavaScript Object to Long";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
  SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_006;

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
}

function MyFunction() {
  return;
}
MyFunction.valueOf = new Function( "return 6060842" );

new TestCase(
  "TEST_CLASS.ambiguous( new String() ) +''",
  "LONG",
  TEST_CLASS.ambiguous(new String()) +'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Boolean() ) +''",
  "LONG",
  TEST_CLASS.ambiguous( new Boolean() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Number() ) +''",
  "LONG",
  TEST_CLASS.ambiguous( new Number() )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new Date(0) ) +''",
  "LONG",
  TEST_CLASS.ambiguous( new Date(0) )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( new MyObject(999) ) +''",
  "LONG",
  TEST_CLASS.ambiguous( new MyObject(999) )+'' );

new TestCase(
  "TEST_CLASS.ambiguous( MyFunction ) +''",
  "LONG",
  TEST_CLASS.ambiguous( MyFunction )+'' );

test();


