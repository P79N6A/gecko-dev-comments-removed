





































gTestfile = '12.6.3-11.js';





































var SECTION = "12.6.3-11";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);





var result = "";

for ( p in Number ) { result += String(p) };

new TestCase( SECTION,
	      "result = \"\"; for ( p in Number ) { result += String(p) };",
	      "",
	      result );

test();

