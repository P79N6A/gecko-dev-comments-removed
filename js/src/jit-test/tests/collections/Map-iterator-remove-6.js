



load(libdir + "asserts.js");

var map = Map();
for (var i = 0; i < 32; i++)
    map.set(i, i);
var iter = map.iterator();
assertEq(iter.next()[0], 0);
for (var i = 0; i < 30; i++)
    map.delete(i);
assertEq(map.size(), 2);
for (var i = 32; i < 100; i++)
    map.set(i, i);  

for (var i = 30; i < 100; i++)
    assertEq(iter.next()[0], i);
assertThrowsValue(function () { iter.next(); }, StopIteration);
