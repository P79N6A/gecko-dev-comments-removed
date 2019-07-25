





var g = newGlobal('new-compartment');
g.eval('function f(a) { debugger; evaluate("debugger;", {newContext: true}); }');

var dbg = new Debugger(g);
var hits = 0;
dbg.onDebuggerStatement = function (frame1) {
    dbg.onDebuggerStatement = function (frame2) {
        assertEq(frame1.eval("a").return, 31);
        hits++;
    };
};

g.f(31);
assertEq(hits, 1);
