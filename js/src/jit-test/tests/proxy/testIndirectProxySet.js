


var p = Proxy.create({
    getOwnPropertyDescriptor() { return undefined; },
    getPropertyDescriptor() { return undefined; },
    defineProperty() { throw "FAIL"; }
});
var q = new Proxy(p, {
    defineProperty(t, id, desc) {
        assertEq(t, p);
        assertEq(id, "x");
        assertEq(desc.configurable, true);
        assertEq(desc.enumerable, true);
        assertEq(desc.writable, true);
        assertEq(desc.value, 3);
        hits++;
    }
});
var hits = 0;




q.x = 3;
assertEq(hits, 1);
