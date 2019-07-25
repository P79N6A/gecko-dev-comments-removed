








var g = newGlobal('new-compartment');
g.eval("var d = 0;\n" +
       "function f() { return 'ok'; }\n" +
       "trap(f, 0, Array(17).join('\\n') + 'd++;');\n");

var dbg = new Debugger;
var gw = dbg.addDebuggee(g);
var fw = gw.getOwnPropertyDescriptor("f").value;
var bp = {hits: 0, hit: function (frame) { this.hits++; }};
fw.script.setBreakpoint(0, bp);

gc();

g.f();
assertEq(g.d, 1);
assertEq(bp.hits, 1);
