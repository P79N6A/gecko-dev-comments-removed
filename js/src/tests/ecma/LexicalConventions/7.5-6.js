





































gTestfile = '7.5-6.js';












var SECTION = "7.5-6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,    "var _0abc = 5",   5,    eval("var _0abc = 5; _0abc") );

test();
