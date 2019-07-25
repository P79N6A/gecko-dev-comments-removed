


var g = newGlobal('new-compartment');
var hit = false;
var dbg = new Debugger(g);
dbg.hooks = {debuggerHandler: function (stack) { hit = 'fail';}};

delete dbg.hooks.debuggerHandler;
g.eval("debugger;");
assertEq(hit, false);

dbg.hooks.debuggerHandler = null;
g.eval("debugger;");
assertEq(hit, false);


dbg.hooks.debuggerHandler = function (stack) { hit = true; };
g.eval("debugger;");
assertEq(hit, true);
