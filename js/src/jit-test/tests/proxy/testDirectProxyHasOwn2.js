



var target = {};
var called = false;
var handler = {
    hasOwn: function (target1, name) {
        assertEq(this, handler);
        assertEq(target1, target);
        assertEq(name, 'foo');
        called = true;
    }
};
({}).hasOwnProperty.call(new Proxy(target, handler), 'foo');
assertEq(called, true);
