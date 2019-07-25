load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: false
});
var caught = false;
assertThrowsInstanceOf(function () {
    'foo' in new Proxy(target, {
        has: function (target, name) {
            return false;
        }
    });
}, TypeError);
