load(libdir + "asserts.js");





assertThrowsInstanceOf(function () {
    Object.getOwnPropertyDescriptor(new Proxy({}, {
        getOwnPropertyDescriptor: function (target, name) {
            return {
                configurable: false
            };
        }
    }), 'foo');
}, TypeError);
