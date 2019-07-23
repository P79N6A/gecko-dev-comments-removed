





































gTestfile = '15.9.5.3-1-n.js';
















var SECTION = "15.9.5.3-1-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.valueOf";

writeHeaderToLog( SECTION + " "+ TITLE);

var OBJ = new MyObject( new Date(0) );

DESCRIPTION = "var OBJ = new MyObject( new Date(0) ); OBJ.valueOf()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var OBJ = new MyObject( new Date(0) ); OBJ.valueOf()",
	      "error",
	      eval("OBJ.valueOf()") );
test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = Date.prototype.valueOf;


  return this;
}
