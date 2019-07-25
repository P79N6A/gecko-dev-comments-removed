

var g = newGlobal('new-compartment');
var dbg1 = Debug(g);
var dbg2 = Debug();
g.parent = this;
g.eval("parent.dbg2.addDebuggee(this);");
