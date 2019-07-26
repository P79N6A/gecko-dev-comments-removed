


assertEq(Math.atanh(NaN), NaN);


assertEq(Math.atanh(-ONE_PLUS_EPSILON), NaN);
assertEq(Math.atanh(-Number.MAX_VALUE), NaN);
assertEq(Math.atanh(-Infinity), NaN);

for (var i = -5; i < -1; i += 0.1)
    assertEq(Math.atanh(i), NaN);


assertEq(Math.atanh(ONE_PLUS_EPSILON), NaN);
assertEq(Math.atanh(Number.MAX_VALUE), NaN);
assertEq(Math.atanh(Infinity), NaN);

for (var i = +5; i > +1; i -= 0.1)
    assertEq(Math.atanh(i), NaN);


assertEq(Math.atanh(-1), -Infinity);


assertEq(Math.atanh(+1), Infinity);


assertEq(Math.atanh(+0), +0);


assertEq(Math.atanh(-0), -0);


reportCompare(0, 0, "ok");
