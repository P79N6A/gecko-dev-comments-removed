





































gTestfile = '7.5-7.js';












var SECTION = "7.5-7";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,    "var $0abc = 5",   5,    eval("var $0abc = 5; $0abc") );

test();
