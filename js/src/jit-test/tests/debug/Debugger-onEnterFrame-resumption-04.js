

var g = newGlobal();
var dbg = Debugger(g);
var hits = 0;
var savedFrame;
dbg.onEnterFrame = function (frame) {
    hits++;
    savedFrame = frame;
    return undefined;
};

savedFrame = undefined;
assertEq(g.eval("'pass';"), "pass");
assertEq(savedFrame.live, false);
assertEq(hits, 1);
