





































gTestfile = '15.4.4.js';
















var SECTION = "15.4.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Array Prototype Object";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,	"Array.prototype.length",   0,          Array.prototype.length );


new TestCase( SECTION,	"typeof Array.prototype",    "object",   typeof Array.prototype );

new TestCase( SECTION,
	      "Array.prototype.toString = Object.prototype.toString; Array.prototype.toString()",
	      "[object Array]",
	      eval("Array.prototype.toString = Object.prototype.toString; Array.prototype.toString()") );

test();
