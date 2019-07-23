





































gTestfile = '12.6.3-6-n.js';





































var SECTION = "12.6.3-4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);




DESCRIPTION = "bad left-hand side expression";
EXPECTED = "error";

new TestCase( SECTION,
	      "bad left-hand side expression",
	      "error",
	      eval("var o = new MyObject(); var result = 0; for ( this in o) { result += this[p]; }") );









test();

function MyObject() {
  this.value = 2;
  this[0] = 4;
  return this;
}
