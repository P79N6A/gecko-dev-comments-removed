

load(libdir + "iteration.js");

var s = Set();
var it = s[std_iterator]();
assertIteratorResult(it.next(), undefined, true);  
s.clear();
s.add("a");
assertIteratorResult(it.next(), undefined, true);
