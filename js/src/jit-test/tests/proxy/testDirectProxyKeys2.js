



var target = {};
var called = false;
var handler = {
    keys: function (target1) {
        assertEq(this, handler);
        assertEq(target1, target);
        called = true;
        return [];
    }
};
Object.keys(new Proxy(target, handler));
assertEq(called, true);
