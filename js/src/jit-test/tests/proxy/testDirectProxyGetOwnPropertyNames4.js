load(libdir + "asserts.js");


assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy({}, {
        ownKeys: function (target) {
            return [ 'foo', 'foo' ];
        }
    }));
}, TypeError);
