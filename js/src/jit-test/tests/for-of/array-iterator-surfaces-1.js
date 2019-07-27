

load(libdir + "iteration.js");

var constructors = [Array, String, Uint8Array, Uint8ClampedArray];
for (var c of constructors) {
    assertEq(c.prototype[std_iterator].length, 0);

    var loc = (c === Array || c === String)
            ? c.prototype
            : Object.getPrototypeOf(c.prototype);

    var desc = Object.getOwnPropertyDescriptor(loc, std_iterator);
    assertEq(desc.configurable, true);
    assertEq(desc.enumerable, false);
    assertEq(desc.writable, true);
}
