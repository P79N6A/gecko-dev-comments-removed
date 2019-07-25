

var g = newGlobal('new-compartment');
g.eval("var line0 = Error().lineNumber;\n" +
       "function f() {\n" +     
       "    return 2;\n" +      
       "}\n");

var N = 4;
var hits = 0;
for (var i = 0; i < N; i++) {
    var dbg = Debug(g);
    dbg.hooks = {
        debuggerHandler: function (frame) {
            var handler = {hit: function () { hits++; }};
            var s = frame.eval("f").return.script;
            var offs = s.getLineOffsets(g.line0 + 2);
            for (var j = 0; j < offs.length; j++)
                s.setBreakpoint(offs[j], handler);
        }
    }
    g.eval('debugger;');
    dbg.hooks = {};
    dbg = null;
}

gc();

assertEq(g.f(), 2);
assertEq(hits, N);
