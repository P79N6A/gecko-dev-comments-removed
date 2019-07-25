load(libdir + "asserts.js");





var target = {};
Object.preventExtensions(target);
assertThrowsInstanceOf(function () {
    ({}).hasOwnProperty.call(new Proxy(target, {
        hasOwn: function (target, name) {
            return true;
        }
    }), 'foo');
}, TypeError);
