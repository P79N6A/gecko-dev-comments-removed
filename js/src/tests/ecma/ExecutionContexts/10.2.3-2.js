





































gTestfile = '10.2.3-2.js';


















var SECTION = "10.2.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Function and Anonymous Code";

writeHeaderToLog( SECTION + " "+ TITLE);

var o = new MyObject("hello");

new TestCase( SECTION,
	      "MyFunction(\"PASSED!\")",
	      "PASSED!",
	      MyFunction("PASSED!") );

var o = MyFunction();

new TestCase( SECTION,
	      "MyOtherFunction(true);",
	      false,
	      MyOtherFunction(true) );

test();

function MyFunction( value ) {
  var x = value;
  delete x;
  return x;
}
function MyOtherFunction(value) {
  var x = value;
  return delete x;
}
function MyObject( value ) {
  this.THIS = this;
}
