





options("typeinfer");
gczeal(2);
var g = newGlobal('new-compartment');
gcparam("maxBytes", 22000);
let array = g.eval("new Array(1,2,3)");
