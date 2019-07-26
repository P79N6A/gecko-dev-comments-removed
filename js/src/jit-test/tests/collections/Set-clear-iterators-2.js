

load(libdir + "iteration.js");

var s = Set(["a"]);
var it = s[std_iterator]();
assertIteratorNext(it, "a");
s.clear();
s.add("b");
assertIteratorNext(it, "b");
assertIteratorDone(it, undefined);
