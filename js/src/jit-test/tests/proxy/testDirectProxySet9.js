


var t = {x: 1};
var p = new Proxy(t, {
    defineProperty(t, id, desc) {
        hits++;
        assertEq(desc.enumerable, true);
        assertEq(desc.configurable, true);
        assertEq(desc.writable, true);
        assertEq(desc.value, 42);
    }
});
var hits = 0;
p.x = 42;
assertEq(hits, 1);
assertEq(t.x, 1);
