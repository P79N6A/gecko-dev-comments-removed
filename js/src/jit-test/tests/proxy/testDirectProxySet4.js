load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    get: function () {
        return 'bar'
    },
    configurable: false
});
assertThrowsInstanceOf(function () {
    new Proxy(target, {
        set: function (target, name, val, receiver) {
            return true;
        }
    })['foo'] = 'baz';
}, TypeError);
