













var HOTLOOP = this.tracemonkey ? tracemonkey.HOTLOOP : 8;
var a;
function f(n) {
    for (var i = 0; i < HOTLOOP; i++)
        if (i == HOTLOOP - 2)
            a = this;
}








f.call("s", 1);
f.call("s", 1);
f.call("s", 1);
f.call("s", 1);
f.call("s", 1);
f.call("s", 1);
f.call("s", 1);
f.call("s", 1);
assertEq(typeof a, "object");
assertEq("" + a, "s");
