
function timeoutfunc() {}
timeout(1, timeoutfunc);
var g = newGlobal('new-compartment');
var dbg = Debugger(g);
