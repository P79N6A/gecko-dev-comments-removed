

var g = newGlobal('new-compartment');
var dbg = new Debugger(g);
g.eval('function f() { debugger; }');
var log;
dbg.onEnterFrame = function handleEnterFrame(f) {
    log += '(';
    assertEq(f.callee.name, 'f');
    f.onPop = function handlePop(c) {
        log += ')';
        assertEq(dbg.enabled, true);
    };
};

var enable;
dbg.onDebuggerStatement = function handleDebugger(f) {
    dbg.enabled = enable;
}



log = 'a';
enable = true;
g.f();


log += 'b';
enable = false;
g.f();


log += 'c';
dbg.enabled = false;
enable = false;
g.f();


log += 'd';
dbg.enabled = true;
enable = true;
g.f();

assertEq(log, 'a()b(cd()');
