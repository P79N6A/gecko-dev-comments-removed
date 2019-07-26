



load(libdir + "iteration.js");

var set = Set();
for (var i = 0; i < 32; i++)
    set.add(i);
var iter = set[std_iterator]();
assertIteratorResult(iter.next(), 0, false);
for (var i = 0; i < 30; i++)
    set.delete(i);
assertEq(set.size, 2);
for (var i = 32; i < 100; i++)
    set.add(i);  

for (var i = 30; i < 100; i++)
    assertIteratorResult(iter.next(), i, false);
assertIteratorResult(iter.next(), undefined, true);
