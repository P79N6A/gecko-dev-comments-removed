


load(libdir + "asserts.js");

var data = [["one", 1], ["two", 2], ["three", 3], ["four", 4]];
var m = Map(data);

var ki = m.keys();
assertEq(ki.next(), "one");
assertEq(ki.next(), "two");
assertEq(ki.next(), "three");
assertEq(ki.next(), "four");
assertThrowsValue(function () { ki.next(); }, StopIteration);

assertEq([k for (k of m.keys())].toSource(), ["one", "two", "three", "four"].toSource());
assertEq([k for (k of m.values())].toSource(), [1, 2, 3, 4].toSource());
assertEq([k for (k of m.entries())].toSource(), data.toSource());
