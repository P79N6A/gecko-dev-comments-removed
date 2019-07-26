



var target = {};
Object.preventExtensions(target);
assertEq(
    'foo' in new Proxy(target, {
        has: function (target, name) {
            return false;
        }
    }), false);
