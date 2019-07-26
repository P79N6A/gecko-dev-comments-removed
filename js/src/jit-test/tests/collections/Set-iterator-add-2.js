

load(libdir + "iteration.js");

var set = Set();
var iter0 = set[std_iterator](), iter1 = set[std_iterator]();
assertIteratorResult(iter0.next(), undefined, true);  
set.add("x");
assertIteratorResult(iter0.next(), undefined, true);  
assertIteratorResult(iter1.next(), "x", false);  
