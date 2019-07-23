





































gTestfile = '15.2.2.2.js';























var SECTION = "15.2.2.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "new Object()";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "typeof new Object()",   "object",       typeof new Object() );
new TestCase( SECTION, "Object.prototype.toString()",   "[object Object]",  Object.prototype.toString() );
new TestCase( SECTION, "(new Object()).toString()",  "[object Object]",   (new Object()).toString() );

test();
