load(libdir + "asserts.js");


assertThrowsInstanceOf(function () {
    Object.preventExtensions(new Proxy({}, {
        preventExtensions: function () {
            return true;
        }
    }));
}, TypeError);
