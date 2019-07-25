load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    value: 'bar',
    writable: false,
    configurable: false
});
assertThrowsInstanceOf(function () {
    new Proxy(target, {
        get: function (target, name, receiver) {
            return 'baz';
        }
    })['foo'];
}, TypeError);
