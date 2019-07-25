

var g2arr = []; 
var xarr = []; 

var N = 4, M = 4;
for (var i = 0; i < N; i++) {
    var g1 = newGlobal('new-compartment');
    g1.M = M;
    var dbg = new Debug(g1);
    var g2 = g1.eval("newGlobal('same-compartment')");
    g1.x = g2.eval("x = {};");

    dbg.hooks = {debuggerHandler: function (frame) { xarr.push(frame.eval("x").return); }};
    g1.eval("debugger;");
    g2arr.push(g2);

    g1 = null;
    gc();
}



assertEq(g2arr.length, N);
assertEq(xarr.length, N);





for (var i = 0; i < N; i++) {
    var obj = xarr[i];
    for (j = 0; j < M; j++) {
        assertEq(obj instanceof Debug.Object, true);
        g2arr[i].eval("x = x.__proto__ = {};");
        obj = obj.proto;
        assertEq("seen" in obj, false);
        obj.seen = true;
        gc();
    }
}
