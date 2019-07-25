

var g = newGlobal('new-compartment');
var dbg = Debugger();
var gobj = dbg.addDebuggee(g);
g.self = g;
var desc = gobj.getOwnPropertyDescriptor("self");
assertEq(desc.value, gobj);
