


assertEq(Math.cosh(NaN), NaN);


assertEq(Math.cosh(+0), 1);


assertEq(Math.cosh(-0), 1);


assertEq(Math.cosh(Infinity), Infinity);


assertEq(Math.cosh(-Infinity), Infinity);


reportCompare(0, 0, "ok");
