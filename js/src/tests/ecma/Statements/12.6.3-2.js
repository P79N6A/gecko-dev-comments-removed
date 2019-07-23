





































gTestfile = '12.6.3-2.js';











var SECTION = "12.6.3-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "The for..in statement";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"Boolean.prototype.foo = 34; for ( j in Boolean ) Boolean[j]",
		34,
		eval("Boolean.prototype.foo = 34; for ( j in Boolean ) Boolean[j] ") );

test();
