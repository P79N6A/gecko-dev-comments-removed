





































gTestfile = '15.9.5.js';
























var SECTION = "15.9.5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Date Prototype Object";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Date.prototype.__proto__ == Object.prototype",
	      true,
	      Date.prototype.__proto__ == Object.prototype );
test();

