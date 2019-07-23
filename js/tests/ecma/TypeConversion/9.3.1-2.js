





































gTestfile = '9.3.1-2.js';





















var SECTION = "9.3.1-2";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "ToNumber applied to the String type";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase( SECTION,  "Number(00)",        0,         Number("00"));
new TestCase( SECTION,  "Number(01)",        1,         Number("01"));
new TestCase( SECTION,  "Number(02)",        2,         Number("02"));
new TestCase( SECTION,  "Number(03)",        3,         Number("03"));
new TestCase( SECTION,  "Number(04)",        4,         Number("04"));
new TestCase( SECTION,  "Number(05)",        5,         Number("05"));
new TestCase( SECTION,  "Number(06)",        6,         Number("06"));
new TestCase( SECTION,  "Number(07)",        7,         Number("07"));
new TestCase( SECTION,  "Number(010)",       10,        Number("010"));
new TestCase( SECTION,  "Number(011)",       11,        Number("011"));



new TestCase( SECTION,  "Number(001)",        1,         Number("001"));
new TestCase( SECTION,  "Number(0001)",       1,         Number("0001"));

test();

