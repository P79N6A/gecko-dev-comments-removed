

var g = newGlobal();
var dbg = Debugger(g);
gc(g);
gc(this);
