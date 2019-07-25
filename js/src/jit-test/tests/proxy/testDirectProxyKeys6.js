load(libdir + "asserts.js");


var target = {};
Object.defineProperty(target, 'foo', {
    enumerable: true,
    configurable: false
});
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy(target, {
        getOwnPropertyNames: function (target) {
            return [];
        }
    }));
}, TypeError);
