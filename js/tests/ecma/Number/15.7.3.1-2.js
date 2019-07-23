





































gTestfile = '15.7.3.1-2.js';














var SECTION = "15.7.3.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(   SECTION,
		"var NUM_PROT = Number.prototype; Number.prototype = null; Number.prototype == NUM_PROT",
		true,
		eval("var NUM_PROT = Number.prototype; Number.prototype = null; Number.prototype == NUM_PROT") );

new TestCase(   SECTION,
		"Number.prototype=0; Number.prototype",
		Number.prototype,
		eval("Number.prototype=0; Number.prototype") );

test();
