

var m = new Map;
var key = {};


assertEq(m.delete(key), false);
assertEq(m.has(key), false);


assertEq(m.set(key, 'x'), undefined);
assertEq(m.delete(key), true);
assertEq(m.has(key), false);
assertEq(m.get(key), undefined);


assertEq(m.delete(key), false);
assertEq(m.has(key), false);
