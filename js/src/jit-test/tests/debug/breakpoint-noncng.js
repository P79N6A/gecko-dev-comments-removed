

var g = newGlobal('new-compartment');
g.s = '';
var dbg = new Debugger;
var gw = dbg.addDebuggee(g);
g.evaluateNonCompileAndGo(
    "function f() {\n" +  
    "    s += 'a';\n" +   
    "    s += 'b';\n" +   
    "}\n");

var fscript = gw.makeDebuggeeValue(g.f).script;
var handler = {hit: function (frame) { g.s += '1'; }};
for (var pc of fscript.getLineOffsets(fscript.startLine + 2))
    fscript.setBreakpoint(pc, handler);

g.f();

assertEq(g.s, "a1b");
