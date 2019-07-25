




var target = {};
var called = false;
var handler = {
    set: function (target1, name, val, receiver) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(name, 'foo');
        assertEq(val, 'baz');
        assertEq(receiver, proxy);
        called = true;
    }
};
var proxy = new Proxy(target, handler);
proxy['foo'] = 'baz';
assertEq(called, true);
