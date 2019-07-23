





































gTestfile = '12.6.3-9-n.js';





































var SECTION = "12.6.3-9-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);




DESCRIPTION = "object is not defined";
EXPECTED = "error";

new TestCase( SECTION,
	      "object is not defined",
	      "error",
	      eval("var o = new MyObject(); var result = 0; for ( var o in foo) { result += this[o]; } ") );









test();

function MyObject() {
  this.value = 2;
  this[0] = 4;
  return this;
}
