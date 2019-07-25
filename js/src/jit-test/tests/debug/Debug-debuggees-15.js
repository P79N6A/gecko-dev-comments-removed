
var g = newGlobal('new-compartment');
var dbg = Debug(g);
g.parent = this;
var n = 2;
g.eval("parent.dbg.removeDebuggee(this); parent.n += 2");
assertEq(n, 4);
