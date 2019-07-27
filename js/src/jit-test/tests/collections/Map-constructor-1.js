

var m = new Map();
assertEq(m.size, 0);
m = new Map(undefined);
assertEq(m.size, 0);
m = new Map(null);
assertEq(m.size, 0);
