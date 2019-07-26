


assertEq(Math.expm1(NaN), NaN);


assertEq(Math.expm1(+0), +0);


assertEq(Math.expm1(-0), -0);


assertEq(Math.expm1(Infinity), Infinity);


assertEq(Math.expm1(-Infinity), -1);


reportCompare(0, 0, "ok");

