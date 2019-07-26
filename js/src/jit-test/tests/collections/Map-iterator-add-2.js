

load(libdir + "iteration.js");

var map = Map();
var iter0 = map[std_iterator](), iter1 = map[std_iterator]();
assertIteratorResult(iter0.next(), undefined, true);  
map.set(1, 2);
assertIteratorResult(iter0.next(), undefined, true);  
assertIteratorResult(iter1.next(), [1, 2], false);  
