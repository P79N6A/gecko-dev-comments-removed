



var target = {};
var called = false;
var handler = {
    has: function (target1, name) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(name, 'foo');
        called = true;
    }
};
'foo' in new Proxy(target, handler);
assertEq(called, true);
