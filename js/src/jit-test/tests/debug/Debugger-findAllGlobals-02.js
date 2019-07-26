

var g1 = newGlobal();           
var g2 = newGlobal();           

var dbg = new Debugger;

var g3 = newGlobal();           
var g4 = newGlobal();           

var g1w = dbg.addDebuggee(g1);
var g3w = dbg.addDebuggee(g3);

var a = dbg.findAllGlobals();



var g2w = g1w.makeDebuggeeValue(g2).unwrap();
var g4w = g1w.makeDebuggeeValue(g4).unwrap();
var thisw = g1w.makeDebuggeeValue(this).unwrap();


assertEq(a.indexOf(g1w) != -1, true);
assertEq(a.indexOf(g2w) != -1, true);
assertEq(a.indexOf(g3w) != -1, true);
assertEq(a.indexOf(g4w) != -1, true);
assertEq(a.indexOf(thisw) != -1, true);
