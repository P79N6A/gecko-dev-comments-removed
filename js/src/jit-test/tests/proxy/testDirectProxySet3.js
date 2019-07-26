load(libdir + "asserts.js");


for (var key of ['foo', Symbol.for('quux')]) {
    var target = {};
    Object.defineProperty(target, key, {
        value: 'bar',
        writable: false,
        configurable: false
    });
    assertThrowsInstanceOf(function () {
        new Proxy(target, {
            set: function (target, name, val, receiver) {
                return true;
            }
        })[key] = 'baz';
    }, TypeError);
}
