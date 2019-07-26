load(libdir + "asserts.js");


assertThrowsInstanceOf(function () {
    Object.preventExtensions(new Proxy({}, {
        preventExtensions: function () {
            return false;
        }
    }));
}, TypeError);
