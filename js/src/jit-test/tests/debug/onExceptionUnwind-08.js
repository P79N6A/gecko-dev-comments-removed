

var g = newGlobal();
var dbg = Debugger(g);
var frame;
dbg.onExceptionUnwind = function (f, x) {
    frame = f;
    assertEq(frame.live, true);
    throw 'unhandled';
};
dbg.onDebuggerStatement = function(f) {
    assertEq(f.eval('throw 42'), null);
    assertEq(frame.live, false);
};
g.eval('debugger');


quit(0);
