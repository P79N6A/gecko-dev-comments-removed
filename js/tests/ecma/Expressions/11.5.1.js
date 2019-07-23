





































gTestfile = '11.5.1.js';


































var SECTION = "11.5.1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Applying the * operator");

new TestCase( SECTION,    "Number.NaN * Number.NaN",    Number.NaN,     Number.NaN * Number.NaN );
new TestCase( SECTION,    "Number.NaN * 1",             Number.NaN,     Number.NaN * 1 );
new TestCase( SECTION,    "1 * Number.NaN",             Number.NaN,     1 * Number.NaN );

new TestCase( SECTION,    "Number.POSITIVE_INFINITY * 0",   Number.NaN, Number.POSITIVE_INFINITY * 0 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * 0",   Number.NaN, Number.NEGATIVE_INFINITY * 0 );
new TestCase( SECTION,    "0 * Number.POSITIVE_INFINITY",   Number.NaN, 0 * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "0 * Number.NEGATIVE_INFINITY",   Number.NaN, 0 * Number.NEGATIVE_INFINITY );

new TestCase( SECTION,    "-0 * Number.POSITIVE_INFINITY",  Number.NaN,   -0 * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-0 * Number.NEGATIVE_INFINITY",  Number.NaN,   -0 * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * -0",  Number.NaN,   Number.POSITIVE_INFINITY * -0 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * -0",  Number.NaN,   Number.NEGATIVE_INFINITY * -0 );

new TestCase( SECTION,    "0 * -0",                         -0,         0 * -0 );
new TestCase( SECTION,    "-0 * 0",                         -0,         -0 * 0 );
new TestCase( SECTION,    "-0 * -0",                        0,          -0 * -0 );
new TestCase( SECTION,    "0 * 0",                          0,          0 * 0 );

new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * Number.NEGATIVE_INFINITY",    Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * Number.NEGATIVE_INFINITY",    Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * Number.POSITIVE_INFINITY",    Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * Number.POSITIVE_INFINITY",    Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY * Number.POSITIVE_INFINITY );

new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * 1 ",                          Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY * 1 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY * -1 ",                         Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY * -1 );
new TestCase( SECTION,    "1 * Number.NEGATIVE_INFINITY",                           Number.NEGATIVE_INFINITY,   1 * Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "-1 * Number.NEGATIVE_INFINITY",                          Number.POSITIVE_INFINITY,   -1 * Number.NEGATIVE_INFINITY );

new TestCase( SECTION,    "Number.POSITIVE_INFINITY * 1 ",                          Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY * 1 );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY * -1 ",                         Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY * -1 );
new TestCase( SECTION,    "1 * Number.POSITIVE_INFINITY",                           Number.POSITIVE_INFINITY,   1 * Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-1 * Number.POSITIVE_INFINITY",                          Number.NEGATIVE_INFINITY,   -1 * Number.POSITIVE_INFINITY );

test();

