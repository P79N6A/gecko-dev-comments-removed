

var f = (a=0) => a + 1;
assertEq(f(), 1);
assertEq(f(50), 51);
