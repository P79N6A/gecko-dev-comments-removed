load(libdir + "asserts.js");





var target = {};
Object.defineProperty(target, 'foo', {
    configurable: true
});
Object.preventExtensions(target);
var caught = false;
assertThrowsInstanceOf(function () {
    ({}).hasOwnProperty.call(new Proxy(target, {
        hasOwn: function (target, name) {
            return false;
        }
    }), 'foo');
}, TypeError);
