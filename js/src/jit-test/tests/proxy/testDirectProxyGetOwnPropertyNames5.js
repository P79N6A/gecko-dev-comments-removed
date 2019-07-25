load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy(target, {
        getOwnPropertyNames: function (target) {
            return [ 'foo' ];
        }
    }));
}, TypeError);
