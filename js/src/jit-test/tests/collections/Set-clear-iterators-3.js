

load(libdir + "iteration.js");

var s = Set();
var it = s[Symbol.iterator]();
assertIteratorDone(it, undefined);  
s.clear();
s.add("a");
assertIteratorDone(it, undefined);
