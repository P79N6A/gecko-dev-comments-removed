





































gTestfile = '15.6.4-2.js';


















var VERSION = "ECMA_2"
  startTest();
var SECTION = "15.6.4-2";

writeHeaderToLog( SECTION + " Properties of the Boolean Prototype Object");

new TestCase( SECTION, "Boolean.prototype.__proto__",               Object.prototype,       Boolean.prototype.__proto__ );

test();
