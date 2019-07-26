




var target = {};
for (var key of ['foo', Symbol.iterator]) {
    var called = false;
    var handler = {
        get: function (target1, name, receiver) {
            assertEq(this, handler);
            assertEq(target1, target);
            assertEq(name, key);
            assertEq(receiver, proxy);
            called = true;
        }
    };
    var proxy = new Proxy(target, handler);
    assertEq(proxy[key], undefined);
    assertEq(called, true);
}
