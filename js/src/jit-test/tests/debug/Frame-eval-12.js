

var g = newGlobal('new-compartment');
var dbg = new Debugger(g);


var hits = 0;
dbg.onDebuggerStatement = function (frame) {
    try {
        frame.older.environment.parent.getVariable('arguments')
    } catch (e) {
        assertEq(''+e, "Error: Debugger scope is not live");
        hits++;
    }
};
g.eval("function h() { debugger; }");
g.eval("function f() { var x = 0; return function() { x++; h() } }");
g.eval("f('ponies')()");
assertEq(hits, 1);
