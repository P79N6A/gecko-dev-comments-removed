




var target = {};
var called = false;
var handler = {
    defineProperty: function (target1, name, desc1) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(name, 'foo');
        assertEq(desc1 == desc, false);
        assertEq(desc1.value, 'bar');
        assertEq(desc1.writable, true);
        assertEq(desc1.enumerable, false);
        assertEq(desc1.configurable, true);
        called = true;
    }
}
var desc = {
    value: 'bar',
    writable: true,
    enumerable: false,
    configurable: true
};
Object.defineProperty(new Proxy(target, handler), 'foo', desc);
assertEq(called, true);
