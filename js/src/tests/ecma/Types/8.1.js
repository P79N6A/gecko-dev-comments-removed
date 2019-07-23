





































gTestfile = '8.1.js';













var SECTION = "8.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The undefined type";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var x; typeof x",
	      "undefined",
	      eval("var x; typeof x") );

new TestCase( SECTION,
	      "var x; typeof x == 'undefined",
	      true,
	      eval("var x; typeof x == 'undefined'") );

new TestCase( SECTION,
	      "var x; x == void 0",
	      true,
	      eval("var x; x == void 0") );
test();

