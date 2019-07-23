





































gTestfile = '12.10.js';









var SECTION = "12.10-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The with statement";

writeHeaderToLog( SECTION +" "+ TITLE);

new TestCase(   SECTION,
		"var x; with (7) x = valueOf(); typeof x;",
		"number",
		eval("var x; with(7) x = valueOf(); typeof x;") );

test();
