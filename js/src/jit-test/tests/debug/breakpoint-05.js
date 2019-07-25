

load(libdir + "asserts.js");

var g = newGlobal('new-compartment');
var dbg = new Debugger(g);
var hits = 0;
dbg.onDebuggerStatement = function (frame) {
    
    assertThrowsInstanceOf(
        function () {
            for (var i = 0; i < frame.offset; i++)
                frame.script.setBreakpoint(i, {});
        },
        Error);
    hits++;
};
g.eval("x = 256; debugger;");
assertEq(hits, 1);
