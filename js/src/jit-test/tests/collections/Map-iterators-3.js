

load(libdir + "asserts.js");

var m = Map();
var it = m.iterator();
assertThrowsValue(it.next.bind(it), StopIteration);  
m.clear();
m.set("a", 1);
assertThrowsValue(it.next.bind(it), StopIteration);
