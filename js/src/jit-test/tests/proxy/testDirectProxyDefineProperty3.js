load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.defineProperty(new Proxy(target, {
        defineProperty: function (target, name, desc) {
            return true;
        }
    }), 'foo', {
        configurable: true
    });
}, TypeError);
