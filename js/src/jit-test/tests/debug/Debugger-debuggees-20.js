

var g1 = newGlobal();           
var g2 = newGlobal();           

var dbg = new Debugger;

var g3 = newGlobal();           
var g4 = newGlobal();           

var g1w = dbg.addDebuggee(g1);
var g3w = dbg.addDebuggee(g3);
assertEq(dbg.addAllGlobalsAsDebuggees(), undefined);



assertEq(g1w, g3w.makeDebuggeeValue(g1).unwrap());
assertEq(g3w, g1w.makeDebuggeeValue(g3).unwrap());

var g2w = g1w.makeDebuggeeValue(g2).unwrap();
var g4w = g1w.makeDebuggeeValue(g4).unwrap();

var thisw = g1w.makeDebuggeeValue(this).unwrap();


assertEq(dbg.hasDebuggee(g1w), true);
assertEq(dbg.hasDebuggee(g2w), true);
assertEq(dbg.hasDebuggee(g3w), true);
assertEq(dbg.hasDebuggee(g4w), true);

assertEq(dbg.hasDebuggee(thisw), false);
