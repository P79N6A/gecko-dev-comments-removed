


var g = newGlobal('new-compartment');
g.debuggeeGlobal = this;
g.eval("var dbg = new Debugger(debuggeeGlobal);" +
       "dbg.hooks = {debuggerHandler: function () { return {return: '!'}; }};");

function gen() {
    yield '1';
    debugger;  
    yield '2';
}
var x = [v for (v in gen())];
assertEq(x.join(","), "1");
