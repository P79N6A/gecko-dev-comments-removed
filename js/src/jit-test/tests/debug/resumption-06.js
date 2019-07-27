

load(libdir + 'asserts.js')
load(libdir + 'iteration.js')

var g = newGlobal();
g.debuggeeGlobal = this;
g.eval("var dbg = new Debugger(debuggeeGlobal);" +
       "dbg.onDebuggerStatement = function () { return {return: '!'}; };");

function* gen() {
    yield '1';
    debugger;  
    yield '2';
}
var iter = gen();
assertIteratorNext(iter, '1');
assertEq(iter.next(), '!');
assertIteratorDone(iter);
