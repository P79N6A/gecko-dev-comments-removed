


load(libdir + "asserts.js");

var data = [1, 2, 3, 4];
var s = Set(data);

var ki = s.keys();
assertEq(ki.next(), 1);
assertEq(ki.next(), 2);
assertEq(ki.next(), 3);
assertEq(ki.next(), 4);
assertThrowsValue(function () { ki.next(); }, StopIteration);

assertEq([...s.keys()].toSource(), data.toSource());
assertEq([...s.values()].toSource(), data.toSource());
assertEq([...s.entries()].toSource(), [[1, 1], [2, 2], [3, 3], [4, 4]].toSource());
