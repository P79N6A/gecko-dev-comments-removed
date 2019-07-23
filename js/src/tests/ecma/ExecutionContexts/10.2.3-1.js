





































gTestfile = '10.2.3-1.js';


















var SECTION = "10.2.3-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Eval Code";

writeHeaderToLog( SECTION + " "+ TITLE);

var o = new MyObject("hello")

  new TestCase( SECTION,
		"var o = new MyObject('hello'); o.THIS == x",
		true,
		o.THIS == o );

var o = MyFunction();

new TestCase( SECTION,
	      "var o = MyFunction(); o == this",
	      true,
	      o == this );

test();

function MyFunction( value ) {
  return this;
}
function MyObject( value ) {
  this.THIS = this;
}
