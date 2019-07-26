

load(libdir + "referencesVia.js");

var s = Set();
var obj = {};
s.add(obj);
assertEq(referencesVia(s, "key", obj), true);
s.clear();
assertEq(referencesVia(s, "key", obj), false);
