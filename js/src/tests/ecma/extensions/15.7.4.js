





































gTestfile = '15.7.4.js';
























var SECTION = "15.7.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Number Prototype Object";

writeHeaderToLog( SECTION + " "+TITLE);

new TestCase( SECTION,
	      "Number.prototype.toString=Object.prototype.toString;Number.prototype.toString()",
	      "[object Number]",
	      eval("Number.prototype.toString=Object.prototype.toString;Number.prototype.toString()") );

new TestCase( SECTION,
	      "typeof Number.prototype",  
	      "object",
	      typeof Number.prototype );

new TestCase( SECTION,
	      "Number.prototype.valueOf()", 
	      0,
	      Number.prototype.valueOf() );





test();
