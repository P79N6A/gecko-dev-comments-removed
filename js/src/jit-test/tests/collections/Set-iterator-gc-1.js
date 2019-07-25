

load(libdir + "referencesVia.js");
var key = {};
var set = Set([key]);
var iter = set.iterator();
referencesVia(iter, "**UNKNOWN SLOT 0**", set);
referencesVia(set, "key", key);
