

load(libdir + "iteration.js");

var m = Map();
var it = m[std_iterator]();
assertIteratorDone(it, undefined);  
m.clear();
m.set("a", 1);
assertIteratorDone(it, undefined);  
