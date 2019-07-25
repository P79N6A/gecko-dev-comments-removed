load(libdir + "asserts.js");


assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy({}, {
        getOwnPropertyNames: function (target) {
            return [ 'foo', 'foo' ];
        }
    }));
}, TypeError);
