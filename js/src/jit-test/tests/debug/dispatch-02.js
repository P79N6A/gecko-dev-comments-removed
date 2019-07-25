


var g = newGlobal('new-compartment');
var log;

var arr = [];
for (var i = 0; i < 4; i++) {
    arr[i] = new Debugger(g);
    arr[i].num = i;
    arr[i].onDebuggerStatement = function () {
        log += this.num;
        
        for (var j = 0; j < arr.length; j++)
            arr[j].enabled = false;
    };
}

log = '';
g.eval("debugger; debugger;");
assertEq(log, '0');
