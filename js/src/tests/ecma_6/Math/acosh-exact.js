


assertEq(Math.acosh(NaN), NaN);


assertEq(Math.acosh(ONE_MINUS_EPSILON), NaN);
assertEq(Math.acosh(Number.MIN_VALUE), NaN);
assertEq(Math.acosh(+0), NaN);
assertEq(Math.acosh(-0), NaN);
assertEq(Math.acosh(-Number.MIN_VALUE), NaN);
assertEq(Math.acosh(-1), NaN);
assertEq(Math.acosh(-Number.MAX_VALUE), NaN);
assertEq(Math.acosh(-Infinity), NaN);

for (var i = -20; i < 1; i++)
    assertEq(Math.acosh(i), NaN);


assertEq(Math.acosh(1), +0);


assertEq(Math.acosh(Number.POSITIVE_INFINITY), Number.POSITIVE_INFINITY);


reportCompare(0, 0, "ok");
