





































gTestfile = '15.9.5.js';
























var SECTION = "15.9.5";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Date Prototype Object";

writeHeaderToLog( SECTION + " "+ TITLE);


Date.prototype.getClass = Object.prototype.toString;

new TestCase( SECTION,
	      "Date.prototype.getClass",
	      "[object Date]",
	      Date.prototype.getClass() );
new TestCase( SECTION,
	      "Date.prototype.valueOf()",
	      NaN,
	      Date.prototype.valueOf() );
test();

