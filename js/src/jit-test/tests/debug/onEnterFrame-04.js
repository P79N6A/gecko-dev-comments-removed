








load(libdir + 'asserts.js');

var g = newGlobal('new-compartment');
g.eval("function f(frame) { n++; return 42; }");
g.n = 0;

var dbg = Debugger(g);



















dbg.onEnterFrame = g.f;


var debuggeeF = dbg.addDebuggee(g.f);


assertEq(debuggeeF.call(), null);


assertEq(g.n, 0);



quit(0);
