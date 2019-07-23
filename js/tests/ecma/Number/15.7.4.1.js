





































gTestfile = '15.7.4.1.js';











var SECTION = "15.7.4.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Number.prototype.constructor";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase(   SECTION,
		"Number.prototype.constructor",
		Number,
		Number.prototype.constructor );
test();
