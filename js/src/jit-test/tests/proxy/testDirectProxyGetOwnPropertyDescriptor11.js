

load(libdir + "asserts.js");

var p = new Proxy({}, {
    getOwnPropertyDescriptor() { return {configurable: true}; }
});
var desc = Object.getOwnPropertyDescriptor(p, "x");
assertDeepEq(desc, {
    configurable: true,
    enumerable: false,
    value: undefined,
    writable: false
});
