





































gTestfile = '15.9.5.2-2-n.js';





















var SECTION = "15.9.5.2-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Date.prototype.toString";

writeHeaderToLog( SECTION + " "+ TITLE);

var OBJ = new MyObject( new Date(0) );

DESCRIPTION = "var OBJ = new MyObject( new Date(0) ); OBJ.toString()";
EXPECTED = "error";

new TestCase( SECTION,
	      "var OBJ = new MyObject( new Date(0) ); OBJ.toString()",
	      "error",
	      eval("OBJ.toString()") );
test();

function MyObject( value ) {
  this.value = value;
  this.valueOf = new Function( "return this.value" );
  this.toString = Date.prototype.toString;
  return this;
}
