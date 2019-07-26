

var dbg = new Debugger;

var gw = null;
dbg.onNewGlobalObject = function (global) {
  assertEq(arguments.length, 1);
  assertEq(this, dbg);
  gw = global;
};
var g = newGlobal();
assertEq(typeof gw, 'object');
assertEq(dbg.addDebuggee(g), gw);



assertEq(gw.makeDebuggeeValue(g), gw);
