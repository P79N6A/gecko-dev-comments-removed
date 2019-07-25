load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    enumerable: true,
    configurable: true
});
Object.preventExtensions(target);
var caught = false;
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy(target, {
        keys: function (target) {
            return [];
        }
    }));
}, TypeError);
