




var target = {};
var keys = ['foo'];
if (typeof Symbol === "function")
    keys.push(Symbol.for('quux'));
for (var key of keys) {
    var handler = { };
    for (let p of [new Proxy(target, handler), Proxy.revocable(target, handler).proxy]) {
        handler.set = function (target1, name, val, receiver) {
            assertEq(this, handler);
            assertEq(target1, target);
            assertEq(name, key);
            assertEq(val, 'baz');
            assertEq(receiver, p);
            called = true;
        }

        var called = false;
        p[key] = 'baz';
        assertEq(called, true);
    }
}
