

load(libdir + "referencesVia.js");
load(libdir + "iteration.js");

var key = {};
var set = Set([key]);
var iter = set[Symbol.iterator]();
referencesVia(iter, "**UNKNOWN SLOT 0**", set);
referencesVia(set, "key", key);
