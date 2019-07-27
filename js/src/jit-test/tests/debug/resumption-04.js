

var g = newGlobal();
g.debuggeeGlobal = this;
g.eval("var dbg = new Debugger(debuggeeGlobal);" +
       "dbg.onDebuggerStatement = function () { return {return: '!'}; };");

function gen() {
    yield '1';
    debugger;  
    yield '2';
}
var x = [v for (v in gen())];
assertEq(x.join(","), "1");
