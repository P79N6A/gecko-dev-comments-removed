





































gTestfile = '11.5.2.js';

































var SECTION = "11.5.2";
var VERSION = "ECMA_1";
var BUGNUMBER="111202";
startTest();

writeHeaderToLog( SECTION + " Applying the / operator");



new TestCase( SECTION,    "Number.NaN / Number.NaN",    Number.NaN,     Number.NaN / Number.NaN );
new TestCase( SECTION,    "Number.NaN / 1",             Number.NaN,     Number.NaN / 1 );
new TestCase( SECTION,    "1 / Number.NaN",             Number.NaN,     1 / Number.NaN );

new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.NaN",    Number.NaN,     Number.POSITIVE_INFINITY / Number.NaN );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.NaN",    Number.NaN,     Number.NEGATIVE_INFINITY / Number.NaN );



new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.NEGATIVE_INFINITY",    Number.NaN,   Number.NEGATIVE_INFINITY / Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.NEGATIVE_INFINITY",    Number.NaN,   Number.POSITIVE_INFINITY / Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.POSITIVE_INFINITY",    Number.NaN,   Number.NEGATIVE_INFINITY / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.POSITIVE_INFINITY",    Number.NaN,   Number.POSITIVE_INFINITY / Number.POSITIVE_INFINITY );



new TestCase( SECTION,    "Number.POSITIVE_INFINITY / 0",   Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY / 0 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / 0",   Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY / 0 );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / -0",  Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY / -0 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / -0",  Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY / -0 );



new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / 1 ",          Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY / 1 );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / -1 ",         Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY / -1 );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / 1 ",          Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY / 1 );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / -1 ",         Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY / -1 );

new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / Number.MAX_VALUE ",          Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY / Number.MAX_VALUE );
new TestCase( SECTION,    "Number.NEGATIVE_INFINITY / -Number.MAX_VALUE ",         Number.POSITIVE_INFINITY,   Number.NEGATIVE_INFINITY / -Number.MAX_VALUE );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / Number.MAX_VALUE ",          Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY / Number.MAX_VALUE );
new TestCase( SECTION,    "Number.POSITIVE_INFINITY / -Number.MAX_VALUE ",         Number.NEGATIVE_INFINITY,   Number.POSITIVE_INFINITY / -Number.MAX_VALUE );



new TestCase( SECTION,    "1 / Number.NEGATIVE_INFINITY",   -0,             1 / Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "1 / Number.POSITIVE_INFINITY",   0,              1 / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-1 / Number.POSITIVE_INFINITY",  -0,             -1 / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-1 / Number.NEGATIVE_INFINITY",  0,              -1 / Number.NEGATIVE_INFINITY );

new TestCase( SECTION,    "Number.MAX_VALUE / Number.NEGATIVE_INFINITY",   -0,             Number.MAX_VALUE / Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "Number.MAX_VALUE / Number.POSITIVE_INFINITY",   0,              Number.MAX_VALUE / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-Number.MAX_VALUE / Number.POSITIVE_INFINITY",  -0,             -Number.MAX_VALUE / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-Number.MAX_VALUE / Number.NEGATIVE_INFINITY",  0,              -Number.MAX_VALUE / Number.NEGATIVE_INFINITY );



new TestCase( SECTION,    "0 / -0",                         Number.NaN,     0 / -0 );
new TestCase( SECTION,    "-0 / 0",                         Number.NaN,     -0 / 0 );
new TestCase( SECTION,    "-0 / -0",                        Number.NaN,     -0 / -0 );
new TestCase( SECTION,    "0 / 0",                          Number.NaN,     0 / 0 );



new TestCase( SECTION,    "0 / 1",                          0,              0 / 1 );
new TestCase( SECTION,    "0 / -1",                        -0,              0 / -1 );
new TestCase( SECTION,    "-0 / 1",                        -0,              -0 / 1 );
new TestCase( SECTION,    "-0 / -1",                       0,               -0 / -1 );



new TestCase( SECTION,    "1 / 0",                          Number.POSITIVE_INFINITY,   1/0 );
new TestCase( SECTION,    "1 / -0",                         Number.NEGATIVE_INFINITY,   1/-0 );
new TestCase( SECTION,    "-1 / 0",                         Number.NEGATIVE_INFINITY,   -1/0 );
new TestCase( SECTION,    "-1 / -0",                        Number.POSITIVE_INFINITY,   -1/-0 );

new TestCase( SECTION,    "0 / Number.POSITIVE_INFINITY",   0,      0 / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "0 / Number.NEGATIVE_INFINITY",   -0,     0 / Number.NEGATIVE_INFINITY );
new TestCase( SECTION,    "-0 / Number.POSITIVE_INFINITY",  -0,     -0 / Number.POSITIVE_INFINITY );
new TestCase( SECTION,    "-0 / Number.NEGATIVE_INFINITY",  0,      -0 / Number.NEGATIVE_INFINITY );

test();

