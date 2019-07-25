
load(libdir + 'asserts.js');

var g = newGlobal('new-compartment');
var dbg = Debugger();
var gw = dbg.addDebuggee(g);

function test(expr, level) {
    var log;
    dbg.onDebuggerStatement = function (frame) {
        log += 'd';
        assertEq(frame.script.staticLevel, level);
    };

    print("Testing: " + expr);

    log = '';
    
    g.evaluate(expr);
    assertEq(log, 'd');
}


test("debugger;", 0);
test("(function () { debugger; })()", 1);
test("(function () { (function () { debugger; })(); })()", 2);
test("(function () { (function () { (function () { debugger; })(); })(); })()", 3);

test("eval('debugger;');", 1);
test("eval('eval(\\\'debugger;\\\');');", 2);
test("evil = eval; eval('evil(\\\'debugger;\\\');');", 0); 
test("(function () { eval('debugger;'); })();", 2);
test("evil = eval; (function () { evil('debugger;'); })();", 0);


test("((function () { debugger; })() for (x in [1])).next();", 2);
test("(x for (x in ((function () { debugger; })(), [1]))).next();", 2);


g.eval('function f() {}');
var fw = gw.makeDebuggeeValue(g.f);
assertThrowsInstanceOf(function () { "use strict"; fw.script.staticLevel = 10; },
                       TypeError);
