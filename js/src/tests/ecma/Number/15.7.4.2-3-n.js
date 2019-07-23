





































gTestfile = '15.7.4.2-3-n.js';



















var SECTION = "15.7.4.2-3-n";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.prototype.toString()");

DESCRIPTION = "o = new String(); o.toString = Number.prototype.toString; o.toString()";
EXPECTED = "error";

new TestCase(SECTION, 
	     "o = new String(); o.toString = Number.prototype.toString; o.toString()", 
	     "error",   
	     eval("o = new String(); o.toString = Number.prototype.toString; o.toString()") );

test();
