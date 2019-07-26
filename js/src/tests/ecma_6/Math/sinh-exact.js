


assertEq(Math.sinh(NaN), NaN);


assertEq(Math.sinh(+0), +0);


assertEq(Math.sinh(-0), -0);


assertEq(Math.sinh(Infinity), Infinity);


assertEq(Math.sinh(-Infinity), -Infinity);


reportCompare(0, 0, "ok");
