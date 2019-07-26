
assertEq(Math.sign(NaN), NaN);


assertEq(Math.sign(-0), -0);


assertEq(Math.sign(+0), +0);


assertEq(Math.sign(-Number.MIN_VALUE), -1);
assertEq(Math.sign(-Number.MAX_VALUE), -1);
assertEq(Math.sign(-Infinity), -1);

for (var i = -1; i > -20; i--)
    assertEq(Math.sign(i), -1);

assertEq(Math.sign(-1e-300), -1);
assertEq(Math.sign(-0x80000000), -1);


assertEq(Math.sign(Number.MIN_VALUE), +1);
assertEq(Math.sign(Number.MAX_VALUE), +1);
assertEq(Math.sign(Infinity), +1);

for (var i = 1; i < 20; i++)
    assertEq(Math.sign(i), +1);

assertEq(Math.sign(+1e-300), +1);
assertEq(Math.sign(0x80000000), +1);
assertEq(Math.sign(0xffffffff), +1);


reportCompare(0, 0, "ok");
