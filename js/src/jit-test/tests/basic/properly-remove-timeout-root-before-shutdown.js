
function timeoutfunc() {}
timeout(1, timeoutfunc);
var g = newGlobal();
var dbg = Debugger(g);
