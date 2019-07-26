load(libdir + "asserts.js");


var target = {};
Object.defineProperty(target, 'foo', {
    configurable: false
});
assertThrowsInstanceOf(function () {
    Object.getOwnPropertyNames(new Proxy(target, {
        ownKeys: function (target) {
            return [];
        }
    }));
}, TypeError);
