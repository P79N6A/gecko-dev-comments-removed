





































gTestfile = '15.2.1.2.js';




























var SECTION = "15.2.1.2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Object()";

writeHeaderToLog( SECTION + " "+ TITLE);

var MYOB = Object();

new TestCase( SECTION, "var MYOB = Object(); MYOB.valueOf()",    MYOB,      MYOB.valueOf()      );
new TestCase( SECTION, "typeof Object()",       "object",               typeof (Object(null)) );
new TestCase( SECTION, "var MYOB = Object(); MYOB.toString()",    "[object Object]",       eval("var MYOB = Object(); MYOB.toString()") );

test();
