load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy(target, {
        keys: function (target) {
            return [ 'foo' ];
        }
    }));
}, TypeError);
