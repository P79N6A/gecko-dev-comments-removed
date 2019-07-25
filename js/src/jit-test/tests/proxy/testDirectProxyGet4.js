load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    set: function (value) {},
    configurable: false
});
assertThrowsInstanceOf(function () {
    new Proxy(target, {
        get: function (target, name, receiver) {
            return 'baz';
        }
    })['foo'];
}, TypeError);
