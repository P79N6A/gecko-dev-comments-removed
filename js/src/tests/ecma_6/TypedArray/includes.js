const constructors = [
    Int8Array,
    Uint8Array,
    Uint8ClampedArray,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Float32Array,
    Float64Array
];

for (var constructor of constructors) {
    if (!("includes" in constructor.prototype))
        break;

    assertEq(constructor.prototype.includes.length, 1);

    assertEq(new constructor([1, 2, 3]).includes(1), true);
    assertEq(new constructor([1, 2, 3]).includes(2), true);
    assertEq(new constructor([1, 2, 3]).includes(3), true);
    assertEq(new constructor([1, 2, 3]).includes(2, 1), true);
    assertEq(new constructor([1, 2, 3]).includes(2, -2), true);
    assertEq(new constructor([1, 2, 3]).includes(2, -100), true);

    assertEq(new constructor([1, 2, 3]).includes("2"), false);
    assertEq(new constructor([1, 2, 3]).includes(2, 2), false);
    assertEq(new constructor([1, 2, 3]).includes(2, -1), false);
    assertEq(new constructor([1, 2, 3]).includes(2, 100), false);

    
    if (typeof newGlobal === "function") {
        var includes = newGlobal()[constructor.name].prototype.includes;
        assertEq(includes.call(new constructor([1, 2, 3]), 2), true);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.includes.call(invalidReceiver);
        }, TypeError, "Assert that reverse fails if this value is not a TypedArray");
    });
    
    constructor.prototype.includes.call(new Proxy(new constructor(), {}));

    
    assertEq(Object.defineProperty(new constructor([1, 2, 3]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).includes(2), true);
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
