



var target = {};
var called = false;
var handler = {
    preventExtensions: function (target1) {
        assertEq(this, handler);
        assertEq(target1, target);
        called = true;
        Object.preventExtensions(target);
        return true;
    }
};
Object.preventExtensions(new Proxy(target, handler));
assertEq(called, true);
