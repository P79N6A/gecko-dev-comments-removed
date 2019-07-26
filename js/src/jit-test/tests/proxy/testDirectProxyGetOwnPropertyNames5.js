load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy(target, {
        ownKeys: function (target) {
            return [ 'foo' ];
        }
    }));
}, TypeError);
