





































gTestfile = '7.2-1.js';


















var SECTION = "7.2-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Line Terminators";

writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase( SECTION,    "var a\nb =  5; ab=10;ab;",     10,     eval("var a\nb =  5; ab=10;ab") );
new TestCase( SECTION,    "var a\nb =  5; ab=10;b;",      5,      eval("var a\nb =  5; ab=10;b") );
new TestCase( SECTION,    "var a\rb =  5; ab=10;ab;",     10,     eval("var a\rb =  5; ab=10;ab") );
new TestCase( SECTION,    "var a\rb =  5; ab=10;b;",      5,      eval("var a\rb =  5; ab=10;b") );
new TestCase( SECTION,    "var a\r\nb =  5; ab=10;ab;",     10,     eval("var a\r\nb =  5; ab=10;ab") );
new TestCase( SECTION,    "var a\r\nb =  5; ab=10;b;",      5,      eval("var a\r\nb =  5; ab=10;b") );

test();
