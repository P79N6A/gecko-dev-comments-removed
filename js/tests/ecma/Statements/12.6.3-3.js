





































gTestfile = '12.6.3-3.js';














var SECTION = "12.6.3-3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);

var o = {};

var result = "";

for ( o.a in [1,2,3] ) { result += String( [1,2,3][o.a] ); }

new TestCase( SECTION,
	      "for ( o.a in [1,2,3] ) { result += String( [1,2,3][o.a] ); } result",
	      "123",
	      result );

test();

