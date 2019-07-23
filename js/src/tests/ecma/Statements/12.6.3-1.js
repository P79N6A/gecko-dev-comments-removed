





































gTestfile = '12.6.3-1.js';










var SECTION = "12.6.3-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "var x; Number.prototype.foo = 34; for ( j in 7 ) x = j; x",
	      "foo",
	      eval("var x; Number.prototype.foo = 34; for ( j in 7 ){x = j;} x") );

test();

