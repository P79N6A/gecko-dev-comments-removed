























var SECTION = "15.7.4.2-2-n";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.prototype.toString()");

DESCRIPTION = "o = new Object(); o.toString = Number.prototype.toString; o.toString()";
EXPECTED = "error";

new TestCase(SECTION, 
	     "o = new Object(); o.toString = Number.prototype.toString; o.toString()", 
	     "error",   
	     eval("o = new Object(); o.toString = Number.prototype.toString; o.toString()") );




test();
