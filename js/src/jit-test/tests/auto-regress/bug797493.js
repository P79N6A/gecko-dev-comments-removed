





var g = newGlobal();
var dbg = new Debugger(g);
dbg.onDebuggerStatement = function handleDebugger(frame) {
    frame.onPop = function handlePop(c) {
      poppedFrames.indexOf(this)
    }
};
g.eval("function g() { for (var i = 0; i < 10; i++) { debugger; yield i; } }");
assertEq(g.eval("var t = 0; for (j in g()) t += j; t;"), 45);
