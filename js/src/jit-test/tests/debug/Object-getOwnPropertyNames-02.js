

var g = newGlobal('new-compartment');
var dbg = Debugger();
var gobj = dbg.addDebuggee(g);
g.p = {xyzzy: 8};  
assertEq(gobj.getOwnPropertyDescriptor("p").value.getOwnPropertyDescriptor("xyzzy").value, 8);
