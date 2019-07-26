

load(libdir + "iteration.js");

var set = Set("abcd");
var iter = set[std_iterator]();
assertIteratorResult(iter.next(), "a", false);
assertIteratorResult(iter.next(), "b", false);
set.delete("c");
set.delete("b");
assertIteratorResult(iter.next(), "d", false);
assertIteratorResult(iter.next(), undefined, true);
