

var g = newGlobal('new-compartment');

var hits = 0;
var handler = {
    hit: function (frame) {
        hits++;
        frame.script.clearBreakpoint(handler);
    }
};

function attach(i) {
    var dbg = Debug(g);
    dbg.hooks = {
        debuggerHandler: function (frame) {
            var s = frame.script;
            var offs = s.getLineOffsets(g.line0 + 2);
            for (var i = 0; i < offs.length; i++)
                s.setBreakpoint(offs[i], handler);
        }
    };
}
for (var i = 0; i < 4; i++)
    attach(i);

g.eval("var line0 = Error().lineNumber;\n" +
       "debugger;\n" +      
       "Math.sin(0);\n");   
assertEq(hits, 4);
