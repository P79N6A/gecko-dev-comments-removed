

load(libdir + "asserts.js");

var p = Proxy.create({ getOwnPropertyDescriptor() { return {}; } });
var desc = Object.getOwnPropertyDescriptor(p, "x");
assertDeepEq(desc, {
    value: undefined,
    writable: false,
    enumerable: false,
    configurable: false
});
