





































gTestfile = '7.5-1.js';












var SECTION = "7.5-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Identifiers";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,    "var $123 = 5",      5,       eval("var $123 = 5;$123") );
new TestCase( SECTION,    "var _123 = 5",      5,       eval("var _123 = 5;_123") );

test();
