


assertEq(Math.cbrt(NaN), NaN);


assertEq(Math.cbrt(+0), +0);


assertEq(Math.cbrt(-0), -0);


assertEq(Math.cbrt(Infinity), Infinity);


assertEq(Math.cbrt(-Infinity), -Infinity);


reportCompare(0, 0, "ok");
