load(libdir + "asserts.js");





assertThrowsInstanceOf(function () {
    Object.preventExtensions(new Proxy({}, {
        preventExtensions: function (target) {
            return false;
        }
    }));
}, TypeError);
