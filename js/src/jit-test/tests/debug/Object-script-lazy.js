











var g1 = newGlobal();
g1.eval('function f() { return "from f"; }');
g1.eval('function g() { return "from g"; }');


var g2 = newGlobal();
var dbg = new Debugger;
var g2w = dbg.addDebuggee(g2);
g2.f = g1.f;
g2.g = g1.g;





var fDO = g2w.getOwnPropertyDescriptor('f').value;
assertEq(fDO.global, g2w);
assertEq(fDO.unwrap().global === g2w, false);
assertEq(fDO.unwrap().script, null);


var gDO = g2w.getOwnPropertyDescriptor('g').value;
assertEq(gDO.global, g2w);
assertEq(gDO.unwrap().global === g2w, false);
assertEq(gDO.unwrap().parameterNames, undefined);


dbg.addDebuggee(g1);
assertEq(fDO.unwrap().script instanceof Debugger.Script, true);
assertEq(gDO.unwrap().parameterNames instanceof Array, true);

