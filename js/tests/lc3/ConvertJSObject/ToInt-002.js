











































var SECTION = "Preferred argument conversion:  JavaScript Object to int";
var VERSION = "1_4";
var TITLE   = "LiveConnect 3.0 JavaScript to Java Data Type Conversion " +
SECTION;
startTest();

var TEST_CLASS = new Packages.com.netscape.javascript.qa.lc3.jsobject.JSObject_007;

function MyFunction() {
    return "hello";
}
MyFunction.valueOf = new Function( "return 999" );

function MyOtherFunction() {
    return "goodbye";
}
MyOtherFunction.valueOf = null;
MyOtherFunction.toString = new Function( "return 999" );

function MyObject(value) {
    this.value = value;
    this.valueOf = new Function("return this.value");
}

function MyOtherObject(stringValue) {
    this.stringValue = String( stringValue );
    this.toString = new Function( "return this.stringValue" );
    this.valueOf = null;
}

function AnotherObject( value ) {
    this.value = value;
    this.valueOf = new Function( "return this.value" );
    this.toString = new Function( "return 666" );
}



new TestCase(
    "TEST_CLASS.ambiguous( MyFunction ) +''",
    "INT",
    TEST_CLASS.ambiguous( MyFunction )+'' );



new TestCase(
    "TEST_CLASS.ambiguous( MyOtherFunction ) +''",
    "INT",
    TEST_CLASS.ambiguous( MyOtherFunction )+'' );



new TestCase(
    "TEST_CLASS.ambiguous( new MyObject(12345) ) +''",
    "INT",
    TEST_CLASS.ambiguous( new MyObject(12345) )+'' );



new TestCase(
    "TEST_CLASS.ambiguous( new MyOtherObject(\"12345\") ) +''",
    "INT",
    TEST_CLASS.ambiguous( new MyOtherObject("12345") )+'' );



new TestCase(
    "TEST_CLASS.ambiguous( new AnotherObject(\"12345\") ) +''",
    "INT",
    TEST_CLASS.ambiguous( new AnotherObject("12345") )+'' );

test();

