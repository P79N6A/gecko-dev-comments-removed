




var g = newGlobal('new-compartment');
var log;
var arr = [];

function addDebug(msg) {
    var dbg = new Debug(g);
    dbg.hooks = {
        debuggerHandler: function (stack) {
            log += msg;
        }
    };
    arr.push(dbg);
}

addDebug('a');
addDebug('b');
addDebug('c');

log = '';
assertEq(g.eval("debugger; 0;"), 0);
assertEq(log, 'abc');




arr[0].hooks = {
    debuggerHandler: function (stack) {
        log += 'a';
        return {return: 1};
    }
};

log = '';
assertEq(g.eval("debugger; 0;"), 1);
assertEq(log, 'a');

reportCompare(0, 0, 'ok');
