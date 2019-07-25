

Object.prototype[1] = "O";
Array.prototype[2] = "A";
assertEq([x for (x of Array(4))].join(","), ",O,A,");
