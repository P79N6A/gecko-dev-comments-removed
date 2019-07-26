


assertEq(Math.tanh(NaN), NaN);


assertEq(Math.tanh(+0), +0);


assertEq(Math.tanh(-0), -0);


assertEq(Math.tanh(Number.POSITIVE_INFINITY), +1);


assertEq(Math.tanh(Number.NEGATIVE_INFINITY), -1);


reportCompare(0, 0, "ok");
