





































gTestfile = '12.6.3-5-n.js';





































var SECTION = "12.6.3-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);




DESCRIPTION = "more than one member expression";
EXPECTED = "error";

new TestCase( SECTION,
	      "more than one member expression",
	      "error",
	      eval("var o = new MyObject(); var result = 0; for ( var i, p in this) { result += this[p]; }") );










test();

function MyObject() {
  this.value = 2;
  this[0] = 4;
  return this;
}
