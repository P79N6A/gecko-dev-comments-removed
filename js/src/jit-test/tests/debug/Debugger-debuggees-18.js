


var dbg = new Debugger;




function assertDebuggees() {
  print("assertDebuggees([" + [g.toSource() for each (g in arguments)] + "])");
  var debuggees = dbg.getDebuggees();
  assertEq(arguments.length, debuggees.length);
  for each (g in arguments)
    assertEq(debuggees.indexOf(g) != -1, true);
}

var g1 = newGlobal(); g1.toSource = function () { return "[global g1]"; };
var g2 = newGlobal(); g2.toSource = function () { return "[global g2]"; };

assertDebuggees();














var dg1 = dbg.addDebuggee(g1);
dg1.toSource = function() { return "[Debugger.Object for global g1]"; };
assertEq(dg1.global, dg1);
assertEq(dg1.unwrap(), dg1);
assertDebuggees(dg1);



var dg2 = dbg.addDebuggee(g2);
dg2.toSource = function() { return "[Debugger.Object for global g2]"; };
assertEq(dg2.global, dg2);
assertEq(dg2.unwrap(), dg2);
assertDebuggees(dg1, dg2);


var dwg1 = dg2.makeDebuggeeValue(g1);
assertEq(dwg1.global, dg2);
assertEq(dwg1.unwrap(), dg1);
dwg1.toSource = function() { return "[Debugger.Object for CCW of global g1]"; };

assertDebuggees(dg1, dg2);
assertEq(dbg.removeDebuggee(g1), undefined);
assertEq(dbg.removeDebuggee(g2), undefined);
assertDebuggees();






















assertEq(dbg.hasDebuggee(g1), false);
assertEq(dbg.hasDebuggee(dg1), false);
assertEq(dbg.hasDebuggee(dwg1), false);

assertEq(dbg.removeDebuggee(g1), undefined); assertDebuggees();
assertEq(dbg.removeDebuggee(dg1), undefined); assertDebuggees();
assertEq(dbg.removeDebuggee(dwg1), undefined); assertDebuggees();







function combo(addAs, designateAs, direct) {
  print("combo(" + uneval(addAs) + ", " + uneval(designateAs) + ")");
  assertDebuggees();
  assertEq(dbg.addDebuggee(addAs), direct);
  assertDebuggees(direct);
  assertEq(dbg.addDebuggee(designateAs), direct);
  assertDebuggees(direct);
  assertEq(dbg.hasDebuggee(designateAs), true);
  assertEq(dbg.removeDebuggee(designateAs), undefined);
  assertDebuggees();
}

combo(g1, g1, dg1);
combo(dg1, g1, dg1);
combo(dwg1, g1, dg1);

combo(g1, dg1, dg1);
combo(dg1, dg1, dg1);
combo(dwg1, dg1, dg1);

combo(g1, dwg1, dg1);
combo(dg1, dwg1, dg1);
combo(dwg1, dwg1, dg1);
