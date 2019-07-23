





































gTestfile = '15.6.3.1.js';



















var SECTION = "15.6.3.1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Boolean.prototype";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,  "Boolean.prototype.valueOf()",       false,   Boolean.prototype.valueOf() );
new TestCase( SECTION,  "Boolean.length",          1,       Boolean.length );

test();
