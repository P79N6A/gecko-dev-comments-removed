


load(libdir + "asserts.js");

var g = newGlobal('new-compartment');
var dbg = Debug(g);
g.parent = this;
assertThrowsInstanceOf(function () { g.eval("parent.dbg.removeDebuggee(this);") }, Error);
