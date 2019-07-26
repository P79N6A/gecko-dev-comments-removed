load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: true
});
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy(target, {
        ownKeys: function (target) {
            return [];
        }
    }));
}, TypeError);
