





var g = newGlobal('new-compartment');
var dbg = new Debugger(g);


dbg.onDebuggerStatement = function (frame) {
    args = frame.eval("arguments");
};
g.eval("function f() { debugger; }");
g.eval("f(this, f, {});");
