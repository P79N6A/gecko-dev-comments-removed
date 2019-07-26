



var target = {};
var called = false;
var handler = {
    preventExtensions: function (target1) {
        assertEq(this, handler);
        assertEq(target1, target);
        Object.preventExtensions(target1);
        called = true;
        return true;
    }
};
Object.preventExtensions(new Proxy(target, handler));
assertEq(called, true);
