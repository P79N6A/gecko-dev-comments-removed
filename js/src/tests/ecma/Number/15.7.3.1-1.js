





































gTestfile = '15.7.3.1-1.js';














var SECTION = "15.7.3.1-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.prototype";

writeHeaderToLog( SECTION +" "+ TITLE);

new TestCase(SECTION,
	     "var NUM_PROT = Number.prototype; delete( Number.prototype ); NUM_PROT == Number.prototype",   
	     true, 
	     eval("var NUM_PROT = Number.prototype; delete( Number.prototype ); NUM_PROT == Number.prototype") );

new TestCase(SECTION,
	     "delete( Number.prototype )",         
	     false,      
	     eval("delete( Number.prototype )") );

test();
