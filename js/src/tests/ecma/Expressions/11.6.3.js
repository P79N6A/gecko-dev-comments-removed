





































gTestfile = '11.6.3.js';




































var SECTION = "11.6.3";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Applying the additive operators (+,-) to numbers");

new TestCase( SECTION,    "Number.NaN + 1",     Number.NaN,     Number.NaN + 1 );
new TestCase( SECTION,    "1 + Number.NaN",     Number.NaN,     1 + Number.NaN );

new TestCase( SECTION,    "Number.NaN - 1",     Number.NaN,     Number.NaN - 1 );
new TestCase( SECTION,    "1 - Number.NaN",     Number.NaN,     1 - Number.NaN );

new TestCase( SECTION,  "Number.POSITIVE_INFINITY + Number.POSITIVE_INFINITY",  Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY + Number.POSITIVE_INFINITY);
new TestCase( SECTION,  "Number.NEGATIVE_INFINITY + Number.NEGATIVE_INFINITY",  Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY + Number.NEGATIVE_INFINITY);

new TestCase( SECTION,  "Number.POSITIVE_INFINITY + Number.NEGATIVE_INFINITY",  Number.NaN,     Number.POSITIVE_INFINITY + Number.NEGATIVE_INFINITY);
new TestCase( SECTION,  "Number.NEGATIVE_INFINITY + Number.POSITIVE_INFINITY",  Number.NaN,     Number.NEGATIVE_INFINITY + Number.POSITIVE_INFINITY);

new TestCase( SECTION,  "Number.POSITIVE_INFINITY - Number.POSITIVE_INFINITY",  Number.NaN,   Number.POSITIVE_INFINITY - Number.POSITIVE_INFINITY);
new TestCase( SECTION,  "Number.NEGATIVE_INFINITY - Number.NEGATIVE_INFINITY",  Number.NaN,   Number.NEGATIVE_INFINITY - Number.NEGATIVE_INFINITY);

new TestCase( SECTION,  "Number.POSITIVE_INFINITY - Number.NEGATIVE_INFINITY",  Number.POSITIVE_INFINITY,   Number.POSITIVE_INFINITY - Number.NEGATIVE_INFINITY);
new TestCase( SECTION,  "Number.NEGATIVE_INFINITY - Number.POSITIVE_INFINITY",  Number.NEGATIVE_INFINITY,   Number.NEGATIVE_INFINITY - Number.POSITIVE_INFINITY);

new TestCase( SECTION,  "-0 + -0",      -0,     -0 + -0 );
new TestCase( SECTION,  "-0 - 0",       -0,     -0 - 0 );

new TestCase( SECTION,  "0 + 0",        0,      0 + 0 );
new TestCase( SECTION,  "0 + -0",       0,      0 + -0 );
new TestCase( SECTION,  "0 - -0",       0,      0 - -0 );
new TestCase( SECTION,  "0 - 0",        0,      0 - 0 );
new TestCase( SECTION,  "-0 - -0",      0,     -0 - -0 );
new TestCase( SECTION,  "-0 + 0",       0,     -0 + 0 );

new TestCase( SECTION,  "Number.MAX_VALUE - Number.MAX_VALUE",      0,  Number.MAX_VALUE - Number.MAX_VALUE );
new TestCase( SECTION,  "1/Number.MAX_VALUE - 1/Number.MAX_VALUE",  0,  1/Number.MAX_VALUE - 1/Number.MAX_VALUE );

new TestCase( SECTION,  "Number.MIN_VALUE - Number.MIN_VALUE",      0,  Number.MIN_VALUE - Number.MIN_VALUE );

test();
