



var target = {};
var called = false;
var handler = {
    ownKeys: function (target1) {
        assertEq(this, handler);
        assertEq(target1, target);
        called = true;
        return [];
    }
};
Object.keys(new Proxy(target, handler));
assertEq(called, true);
