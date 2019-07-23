





































gTestfile = '15.5.4.2.js';










var SECTION = "15.5.4.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "String.prototype.tostring";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "String.prototype.toString.__proto__",  Function.prototype, String.prototype.toString.__proto__ );

test();
