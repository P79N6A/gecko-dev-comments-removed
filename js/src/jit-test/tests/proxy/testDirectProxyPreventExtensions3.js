load(libdir + "asserts.js");





assertThrowsInstanceOf(function () {
    Object.preventExtensions(new Proxy({}, {
        preventExtensions: function (target) {
            return true;
        }
    }));
}, TypeError);
