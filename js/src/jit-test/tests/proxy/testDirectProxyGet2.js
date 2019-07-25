




var target = {};
var called = false;
var handler = {
    get: function (target1, name, receiver) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(name, 'foo');
        assertEq(receiver, proxy);
        called = true;
    }
};
var proxy = new Proxy(target, handler);
proxy['foo'];
assertEq(called, true);
