




for (var i = 0; i < 4; i++) {
    var g = newGlobal('new-compartment');
    var dbg = new Debugger(g);
    dbg.onDebuggerStatement = function () { throw "FAIL"; };
    dbg.o = makeFinalizeObserver();
    dbg.loop = g; 
}

gc();
assertEq(finalizeCount() > 0, true);
