


assertEq(Math.log10(NaN), NaN);


assertEq(Math.log10(-1e-10), NaN);
assertEq(Math.log10(-1e-5), NaN);
assertEq(Math.log10(-1e-1), NaN);
assertEq(Math.log10(-Number.MIN_VALUE), NaN);
assertEq(Math.log10(-Number.MAX_VALUE), NaN);
assertEq(Math.log10(-Infinity), NaN);

for (var i = -1; i > -10; i--)
    assertEq(Math.log10(i), NaN);


assertEq(Math.log10(+0), -Infinity);


assertEq(Math.log10(-0), -Infinity);


assertEq(Math.log10(1), +0);


assertEq(Math.log10(Infinity), Infinity);


reportCompare(0, 0, 'ok');
