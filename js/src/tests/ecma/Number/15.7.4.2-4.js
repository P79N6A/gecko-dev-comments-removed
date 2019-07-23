





































gTestfile = '15.7.4.2-4.js';



















var SECTION = "15.7.4.2-4";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Number.prototype.toString()");

new TestCase(SECTION, 
	     "o = 3; o.toString = Number.prototype.toString; o.toString()",            
	     "3",   
	     eval("o = 3; o.toString = Number.prototype.toString; o.toString()") );

test();
