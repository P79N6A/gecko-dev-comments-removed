





































gTestfile = '12.6.3-10.js';





































var SECTION = "12.6.3-10";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);




var count = 0;
function f() {     count++; return new Array("h","e","l","l","o"); }

var result = "";
for ( p in f() ) { result += f()[p] };

new TestCase( SECTION,
	      "count = 0; result = \"\"; "+
	      "function f() { count++; return new Array(\"h\",\"e\",\"l\",\"l\",\"o\"); }"+
	      "for ( p in f() ) { result += f()[p] }; count",
	      6,
	      count );

new TestCase( SECTION,
	      "result",
	      "hello",
	      result );










test();

