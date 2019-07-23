





































gTestfile = '7.2-6.js';


















var SECTION = "7.2-6";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Line Terminators";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION,    "var a\u000Ab =  5; ab=10;ab;",     10,     eval("var a\nb =  5; ab=10;ab") );
new TestCase( SECTION,    "var a\u000Db =  5; ab=10;b;",      5,      eval("var a\nb =  5; ab=10;b") );

test();
