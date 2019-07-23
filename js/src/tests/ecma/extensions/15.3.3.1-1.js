





































gTestfile = '15.3.3.1-1.js';


















var SECTION = "15.3.3.1-1";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "Function.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "Function.prototype == Function.__proto__",    true, Function.__proto__ == Function.prototype );

test();
