


assertEq(Math.log2(NaN), NaN);


assertEq(Math.log2(-1e-10), NaN);
assertEq(Math.log2(-1e-5), NaN);
assertEq(Math.log2(-1e-1), NaN);
assertEq(Math.log2(-Number.MIN_VALUE), NaN);
assertEq(Math.log2(-Number.MAX_VALUE), NaN);
assertEq(Math.log2(-Infinity), NaN);

for (var i = -1; i > -10; i--)
    assertEq(Math.log2(i), NaN);


assertEq(Math.log2(+0), -Infinity);


assertEq(Math.log2(-0), -Infinity);


assertEq(Math.log2(1), +0);


assertEq(Math.log2(Infinity), Infinity);


reportCompare(0, 0, 'ok');
