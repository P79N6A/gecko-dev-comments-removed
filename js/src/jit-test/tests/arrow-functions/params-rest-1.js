

var A = (...x) => x;
assertEq(A().toSource(), "[]");
assertEq("" + A(3, 4, 5), "3,4,5");
