load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: false
});
assertThrowsInstanceOf(function () {
    ({}).hasOwnProperty.call(Proxy(target, {
        hasOwn: function (target, name) {
            return false;
        }
    }), 'foo');
}, TypeError);
