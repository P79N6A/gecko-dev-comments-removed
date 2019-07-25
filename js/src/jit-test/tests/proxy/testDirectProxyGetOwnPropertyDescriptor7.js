load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyDescriptor(new Proxy(target, {
        getOwnPropertyDescriptor: function (target, name) {
            return {};
        }
    }), 'foo');
}, TypeError);
