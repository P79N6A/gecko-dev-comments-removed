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





assertEq(new Proxy(target, {
        get: function (target, name, receiver) {
            return 'bar';
        }
    })['foo'],
    'bar');






Object.defineProperty(target, 'prop', {
    value: 'bar',
    writable: true,
    configurable: false
});
assertEq(new Proxy(target, {
        get: function (target, name, receiver) {
            return 'baz';
        }
    })['prop'],
    'baz');






Object.defineProperty(target, 'prop2', {
    value: 'bar',
    writable: false,
    configurable: true
});
assertEq(new Proxy(target, {
        get: function (target, name, receiver) {
            return 'baz';
        }
    })['prop2'],
    'baz');
