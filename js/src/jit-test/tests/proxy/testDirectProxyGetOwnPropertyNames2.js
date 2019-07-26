



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
assertEq(Object.getOwnPropertyNames(new Proxy(target, handler)).length, 0);
assertEq(called, true);
