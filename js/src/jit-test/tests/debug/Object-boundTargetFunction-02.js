




var g = newGlobal();
var dbg = new Debugger;
var gw = dbg.addDebuggee(g);
var fw = gw.evalInGlobal("function f() {}; f").return;
assertEq(fw.isBoundFunction, false);
assertEq(fw.boundThis, undefined);
assertEq(fw.boundArguments, undefined);
assertEq(fw.boundTargetFunction, undefined);

var nw = gw.evalInGlobal("var n = Math.max; n").return;
assertEq(nw.isBoundFunction, false);
assertEq(nw.boundThis, undefined);
assertEq(nw.boundArguments, undefined);
assertEq(nw.boundTargetFunction, undefined);

var ow = gw.evalInGlobal("var o = {}; o").return;
assertEq(ow.isBoundFunction, false);
assertEq(ow.boundThis, undefined);
assertEq(ow.boundArguments, undefined);
assertEq(ow.boundTargetFunction, undefined);
