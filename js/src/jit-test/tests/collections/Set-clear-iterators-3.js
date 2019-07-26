

load(libdir + "asserts.js");

var s = Set();
var it = s.iterator();
assertThrowsValue(it.next.bind(it), StopIteration);  
s.clear();
s.add("a");
assertThrowsValue(it.next.bind(it), StopIteration);
