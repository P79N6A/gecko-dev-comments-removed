

var g = newGlobal('new-compartment');
var dbg = Debug(g);
gc(g);
gc(this);
