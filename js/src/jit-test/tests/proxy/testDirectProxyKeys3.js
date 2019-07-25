load(libdir + "asserts.js");


assertThrowsInstanceOf(function () {
    Object.keys(new Proxy({}, {
        keys: function (target) {
            return;
        }
    }));
}, TypeError);
