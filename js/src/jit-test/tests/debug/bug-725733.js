


var g = newGlobal('new-compartment');
g.eval(
    "function f(x) { return {q: x}; }\n" +
    "var n = f('').q;\n");
var dbg = new Debugger(g);
g.eval("f(0)");
