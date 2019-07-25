




var g = newGlobal('new-compartment');
var dbg = new Debug(g);
var rv;
dbg.hooks = {debuggerHandler: function () { throw 15; }};
dbg.uncaughtExceptionHook = function (exc) {
    assertEq(exc, 15);
    return rv;
};


rv = undefined;
g.eval("debugger");


rv = {throw: 57};
var result;
try {
    g.eval("debugger");
    result = 'no exception thrown';
} catch (exc) {
    result = 'caught ' + exc;
}
assertEq(result, 'caught 57');


rv = {return: 42};
assertEq(g.eval("debugger;"), 42);

reportCompare(0, 0, 'ok');
