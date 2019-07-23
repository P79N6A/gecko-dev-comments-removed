





































gTestfile = '15.7.4-1.js';










var SECTION = "15.7.4-1";
var VERSION = "ECMA_1";
startTest();
writeHeaderToLog( SECTION + "Properties of the Number prototype object");

new TestCase(SECTION, "Number.prototype.valueOf()",      0,                  Number.prototype.valueOf() );
new TestCase(SECTION, "typeof(Number.prototype)",        "object",           typeof(Number.prototype) );
new TestCase(SECTION, "Number.prototype.constructor == Number",    true,     Number.prototype.constructor == Number );


test();
