

load(libdir + "asserts.js");

var g = newGlobal('new-compartment');
var dbg = Debug();
var gobj = dbg.addDebuggee(g);
g.eval("function f() { return 2; }");

var s;
dbg.hooks = {debuggerHandler: function (frame) { s = frame.eval("f").return.script; }};
g.eval("debugger;");
assertEq(s.live, true);
s.setBreakpoint(0, {});  

dbg.removeDebuggee(gobj);
assertThrowsInstanceOf(function () { s.setBreakpoint(0, {}); }, Error);
