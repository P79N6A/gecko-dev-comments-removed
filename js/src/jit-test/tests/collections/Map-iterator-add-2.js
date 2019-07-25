

load(libdir + "asserts.js");
load(libdir + "eqArrayHelper.js");

var map = Map();
var iter0 = map.iterator(), iter1 = map.iterator();
assertThrowsValue(function () { iter0.next(); }, StopIteration);  
map.set(1, 2);
assertThrowsValue(function () { iter0.next(); }, StopIteration);  
assertEqArray(iter1.next(), [1, 2]);  
