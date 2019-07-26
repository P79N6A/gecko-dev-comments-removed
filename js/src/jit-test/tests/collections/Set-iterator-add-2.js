

load(libdir + "iteration.js");

var set = Set();
var iter0 = set[std_iterator](), iter1 = set[std_iterator]();
assertIteratorDone(iter0, undefined);  
set.add("x");
assertIteratorDone(iter0, undefined);  
assertIteratorNext(iter1, "x");  
