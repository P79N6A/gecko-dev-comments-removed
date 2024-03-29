

var m = new Map([["a", "b"], ["b", "c"]]);
assertEq(m.size, 2);
m.clear();
assertEq(m.size, 0);
assertEq(m.has("a"), false);
assertEq(m.get("a"), undefined);
assertEq(m.delete("a"), false);
assertEq(m.has("b"), false);
for (var pair of m)
    throw "FAIL";  

m.set("c", "d");
assertEq(m.size, 1);
assertEq(m.has("a"), false);
assertEq(m.has("b"), false);
