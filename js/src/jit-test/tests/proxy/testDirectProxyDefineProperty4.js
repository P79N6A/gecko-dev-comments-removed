load(libdir + "asserts.js");





assertThrowsInstanceOf(function () {
    Object.defineProperty(new Proxy({}, {
        defineProperty: function (target, name, desc) {
            return true;
        }
    }), 'foo', {
        configurable: false
    });
}, TypeError);
