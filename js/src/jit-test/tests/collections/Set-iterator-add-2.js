

load(libdir + "asserts.js");

var set = Set();
var iter0 = set.iterator(), iter1 = set.iterator();
assertThrowsValue(function () { iter0.next(); }, StopIteration);  
set.add("x");
assertThrowsValue(function () { iter0.next(); }, StopIteration);  
assertEq(iter1.next(), "x");  
