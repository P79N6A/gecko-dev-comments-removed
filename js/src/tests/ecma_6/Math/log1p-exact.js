


assertEq(Math.log1p(NaN), NaN);


assertEq(Math.log1p(-1 - 1e-10), NaN);
assertEq(Math.log1p(-1 - 1e-5), NaN);
assertEq(Math.log1p(-1 - 1e-1), NaN);
assertEq(Math.log1p(-ONE_PLUS_EPSILON), NaN);

for (var i = -2; i > -20; i--)
    assertEq(Math.log1p(i), NaN);


assertEq(Math.log1p(-1), -Infinity);


assertEq(Math.log1p(+0), +0);


assertEq(Math.log1p(-0), -0);


assertEq(Math.log1p(Infinity), Infinity);


reportCompare(0, 0, "ok");

