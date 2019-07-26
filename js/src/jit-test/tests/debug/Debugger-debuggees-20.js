

var g1 = newGlobal();           
var g2 = newGlobal();           

var dbg = new Debugger;

var g3 = newGlobal();           
var g4 = newGlobal();           

var g1w = dbg.addDebuggee(g1);
assertEq(dbg.addAllGlobalsAsDebuggees(), undefined);



var g1w = g1w.makeDebuggeeValue(g1).unwrap();
var g2w = g1w.makeDebuggeeValue(g2).unwrap();
var g3w = g1w.makeDebuggeeValue(g3).unwrap();
var g4w = g1w.makeDebuggeeValue(g4).unwrap();
var thisw = g1w.makeDebuggeeValue(this).unwrap();


assertEq(dbg.hasDebuggee(g1w), true);
assertEq(dbg.hasDebuggee(g2w), true);
assertEq(dbg.hasDebuggee(g3w), true);
assertEq(dbg.hasDebuggee(g4w), true);

assertEq(dbg.hasDebuggee(thisw), false);
