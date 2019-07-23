





































gTestfile = '15.7.4.3-2.js';














var SECTION = "15.7.4.3-2";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.prototype.valueOf()");

new TestCase(SECTION,
	     "v = Number.prototype.valueOf; num = 3; num.valueOf = v; num.valueOf()",
	     3, 
	     eval("v = Number.prototype.valueOf; num = 3; num.valueOf = v; num.valueOf()") );

test();
