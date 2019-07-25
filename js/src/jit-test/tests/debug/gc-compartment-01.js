

var g = newGlobal('new-compartment');
var dbg = Debugger(g);
gc(g);
gc(this);
