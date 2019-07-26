

load(libdir + "iteration.js");

var s = Set();
var it = s[std_iterator]();
assertIteratorDone(it, undefined);  
s.clear();
s.add("a");
assertIteratorDone(it, undefined);
