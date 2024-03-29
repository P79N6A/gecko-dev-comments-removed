


load(libdir + "iteration.js");

var data = [["one", 1], ["two", 2], ["three", 3], ["four", 4]];
var m = new Map(data);

var ki = m.keys();
assertIteratorNext(ki, "one");
assertIteratorNext(ki, "two");
assertIteratorNext(ki, "three");
assertIteratorNext(ki, "four");
assertIteratorDone(ki, undefined);

assertEq([k for (k of m.keys())].toSource(), ["one", "two", "three", "four"].toSource());
assertEq([k for (k of m.values())].toSource(), [1, 2, 3, 4].toSource());
assertEq([k for (k of m.entries())].toSource(), data.toSource());
