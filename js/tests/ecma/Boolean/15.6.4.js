





































gTestfile = '15.6.4.js';























var SECTION = "15.6.4";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Properties of the Boolean Prototype Object";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,
	      "Boolean.prototype == false",
	      true,
	      Boolean.prototype == false );

new TestCase( SECTION,
	      "Boolean.prototype.toString = Object.prototype.toString; Boolean.prototype.toString()",
	      "[object Boolean]",
	      eval("Boolean.prototype.toString = Object.prototype.toString; Boolean.prototype.toString()") );

test();
