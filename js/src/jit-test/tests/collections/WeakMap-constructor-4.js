

load(libdir + "asserts.js");
var nonIterables = [true, 1, -0, 3.14, NaN, {}, Math, this];
for (let k of nonIterables)
    assertThrowsInstanceOf(function () { new WeakMap(k); }, TypeError);
