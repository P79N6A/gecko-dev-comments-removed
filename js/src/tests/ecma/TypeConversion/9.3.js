





































gTestfile = '9.3.js';





















var SECTION = "9.3";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "ToNumber";

writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase( SECTION,   "Number()",                      0,              Number() );
new TestCase( SECTION,   "Number(eval('var x'))",         Number.NaN,     Number(eval("var x")) );
new TestCase( SECTION,   "Number(void 0)",                Number.NaN,     Number(void 0) );
new TestCase( SECTION,   "Number(null)",                  0,              Number(null) );
new TestCase( SECTION,   "Number(true)",                  1,              Number(true) );
new TestCase( SECTION,   "Number(false)",                 0,              Number(false) );
new TestCase( SECTION,   "Number(0)",                     0,              Number(0) );
new TestCase( SECTION,   "Number(-0)",                    -0,             Number(-0) );
new TestCase( SECTION,   "Number(1)",                     1,              Number(1) );
new TestCase( SECTION,   "Number(-1)",                    -1,             Number(-1) );
new TestCase( SECTION,   "Number(Number.MAX_VALUE)",      1.7976931348623157e308, Number(Number.MAX_VALUE) );
new TestCase( SECTION,   "Number(Number.MIN_VALUE)",      5e-324,         Number(Number.MIN_VALUE) );

new TestCase( SECTION,   "Number(Number.NaN)",                Number.NaN,                 Number(Number.NaN) );
new TestCase( SECTION,   "Number(Number.POSITIVE_INFINITY)",  Number.POSITIVE_INFINITY,   Number(Number.POSITIVE_INFINITY) );
new TestCase( SECTION,   "Number(Number.NEGATIVE_INFINITY)",  Number.NEGATIVE_INFINITY,   Number(Number.NEGATIVE_INFINITY) );

test();
