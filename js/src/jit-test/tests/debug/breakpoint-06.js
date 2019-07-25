

var g = newGlobal('new-compartment');
var dbg = Debug(g);
var hits = 0;
dbg.hooks = {
    debuggerHandler: function (frame1) {
        function hit(frame2) {
            assertEq(frame2, frame1);
            hits++;
        }
        var s = frame1.script;
        var offs = s.getLineOffsets(g.line0 + 2);
        for (var i = 0; i < offs.length; i++)
            s.setBreakpoint(offs[i], {hit: hit});
    }
};
g.eval("var line0 = Error().lineNumber;\n" +
       "debugger;\n" +  
       "x = 1;\n");     
assertEq(hits, 1);
assertEq(g.x, 1);
