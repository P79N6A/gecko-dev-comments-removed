load(libdir + "asserts.js");


assertThrowsInstanceOf(function () {
    Object.keys(new Proxy({}, {
        ownKeys: function (target) {
            return [ 'foo', 'foo' ];
        }
    }));
}, TypeError);
