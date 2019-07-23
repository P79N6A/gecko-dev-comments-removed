





































gTestfile = '15.7.4.3-3-n.js';














var SECTION = "15.7.4.3-3-n";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.prototype.valueOf()");



DESCRIPTION = "v = Number.prototype.valueOf; o = new String('Infinity'); o.valueOf = v; o.valueOf()";
EXPECTED = "error";

new TestCase("15.7.4.1",
	     "v = Number.prototype.valueOf; o = new String('Infinity'); o.valueOf = v; o.valueOf()",
	     "error", 
	     eval("v = Number.prototype.valueOf; o = new String('Infinity'); o.valueOf = v; o.valueOf()") );



test();
