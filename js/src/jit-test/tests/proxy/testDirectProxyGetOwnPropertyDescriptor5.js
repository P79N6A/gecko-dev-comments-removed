load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: true
});
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyDescriptor(new Proxy(target, {
        getOwnPropertyDescriptor: function (target, name) {
            return undefined;
        }
    }), 'foo');
}, TypeError);
