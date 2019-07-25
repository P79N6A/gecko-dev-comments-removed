

var g = newGlobal('new-compartment');
g.n = 0;
g.eval("function f(frame) { n++; return 42; }");
print('ok');
var dbg = Debugger(g);
dbg.hooks = {enterFrame: g.f};


var x = g.f();
assertEq(x, 42);
assertEq(g.n > 20, true);



quit(0);
