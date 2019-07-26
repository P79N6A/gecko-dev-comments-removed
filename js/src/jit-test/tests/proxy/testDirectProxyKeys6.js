load(libdir + "asserts.js");


var target = {};
Object.defineProperty(target, 'foo', {
    enumerable: true,
    configurable: false
});
assertThrowsInstanceOf(function () {
    Object.keys(new Proxy(target, {
        ownKeys: function (target) {
            return [];
        }
    }));
}, TypeError);
