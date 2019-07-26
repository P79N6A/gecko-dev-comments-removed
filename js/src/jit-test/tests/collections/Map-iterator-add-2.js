

load(libdir + "iteration.js");

var map = Map();
var iter0 = map[std_iterator](), iter1 = map[std_iterator]();
assertIteratorDone(iter0, undefined);  
map.set(1, 2);
assertIteratorDone(iter0, undefined);  
assertIteratorNext(iter1, [1, 2]);     
