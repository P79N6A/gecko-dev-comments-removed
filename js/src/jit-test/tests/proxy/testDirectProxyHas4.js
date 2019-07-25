load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: true
});
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    'foo' in new Proxy(target, {
        has: function (target, name) {
            return false;
        }
    });
}, TypeError);
