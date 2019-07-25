


var dbg = new Debugger;
var g = newGlobal('new-compartment');
var w = dbg.addDebuggee(g);
assertEq(w instanceof Debugger.Object, true);

function usual() {
    assertEq(dbg.hasDebuggee(g), true);
    assertEq(dbg.hasDebuggee(w), true);
    var arr = dbg.getDebuggees();
    assertEq(arr.length, 1);
    assertEq(arr[0], w);
}

usual();
assertEq(dbg.addDebuggee(g), w);
usual();
assertEq(dbg.addDebuggee(w), w);
usual();
dbg.addDebuggee(g.Math);
usual();
dbg.addDebuggee(g.eval("(function () {})"));
usual();


g.g2 = newGlobal('new-compartment');
g.eval("var w2 = new Debugger().addDebuggee(g2)");
dbg.addDebuggee(g.w2);
usual();
assertEq(!dbg.hasDebuggee(g.g2), true);
assertEq(dbg.hasDebuggee(g.w2), true);


assertEq(dbg.removeDebuggee(g), undefined);
assertEq(dbg.hasDebuggee(g), false);
assertEq(dbg.getDebuggees().length, 0);
