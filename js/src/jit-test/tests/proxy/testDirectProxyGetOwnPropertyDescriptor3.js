



var target = {};
var called = false;
var handler = {
    getOwnPropertyDescriptor: function (target1, name) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(name, 'foo');
        called = true;
    }
};
Object.getOwnPropertyDescriptor(new Proxy(target, handler), 'foo');
assertEq(called, true);
