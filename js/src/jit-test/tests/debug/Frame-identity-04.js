


var g = newGlobal('new-compartment');
g.eval("function f(n) { if (n) f(n - 1); debugger; }");
var dbg = new Debugger(g);
var hits = 0;
dbg.hooks = {
    debuggerHandler: function (frame) {
        if (hits === 0) {
            for (; frame; frame = frame.older)
                frame.seen = true;
        } else {
            for (; frame; frame = frame.older)
                assertEq(frame.seen, true);
        }
        gc();
        hits++;
    }
};
g.f(20);
assertEq(hits, 21);
