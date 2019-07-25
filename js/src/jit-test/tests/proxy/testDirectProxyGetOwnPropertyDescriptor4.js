load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: false
});
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyDescriptor(new Proxy(target, {
        getOwnPropertyDescriptor: function (target, name) {
            return undefined;
        }
    }), 'foo');
}, TypeError);
