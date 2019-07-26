load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy(target, {
        ownKeys: function (target) {
            return [ 'foo' ];
        }
    }));
}, TypeError);
