





































gTestfile = '15.6.4-1.js';


















var VERSION = "ECMA_1"
  startTest();
var SECTION = "15.6.4-1";

writeHeaderToLog( SECTION + " Properties of the Boolean Prototype Object");

new TestCase( SECTION, "typeof Boolean.prototype == typeof( new Boolean )", true,          typeof Boolean.prototype == typeof( new Boolean ) );
new TestCase( SECTION, "typeof( Boolean.prototype )",              "object",               typeof(Boolean.prototype) );
new TestCase( SECTION,
	      "Boolean.prototype.toString = Object.prototype.toString; Boolean.prototype.toString()",
	      "[object Boolean]",
	      eval("Boolean.prototype.toString = Object.prototype.toString; Boolean.prototype.toString()") );
new TestCase( SECTION, "Boolean.prototype.valueOf()",               false,                  Boolean.prototype.valueOf() );

test();
