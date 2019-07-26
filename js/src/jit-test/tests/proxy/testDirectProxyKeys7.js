load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    enumerable: true,
    configurable: true
});
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy(target, {
        ownKeys: function (target) {
            return [];
        }
    }));
}, TypeError);
