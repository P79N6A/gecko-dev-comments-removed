





































gTestfile = '15.9.1.13-1.js';

















new TestCase( SECTION,
              "MakeDate(Number.POSITIVE_INFINITY, 0)",
              Number.NaN,
              MakeDate(Number.POSITIVE_INFINITY, 0));

new TestCase( SECTION,
              "MakeDate(Number.NEGATIVE_INFINITY, 0)",
              Number.NaN,
              MakeDate(Number.NEGATIVE_INFINITY, 0));

new TestCase( SECTION,
              "MakeDate(0, Number.POSITIVE_INFINITY)",
              Number.NaN,
              MakeDate(0, Number.POSITIVE_INFINITY));

new TestCase( SECTION,
              "MakeDate(0, Number.NEGATIVE_INFINITY)",
              Number.NaN,
              MakeDate(0, Number.NEGATIVE_INFINITY));

test();

