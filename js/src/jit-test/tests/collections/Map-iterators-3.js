

load(libdir + "iteration.js");

var m = Map();
var it = m[std_iterator]();
assertIteratorResult(it.next(), undefined, true);  
m.clear();
m.set("a", 1);
assertIteratorResult(it.next(), undefined, true);  
