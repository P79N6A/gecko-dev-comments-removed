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
    assertEq(constructor.prototype.values.length, 0);
    assertEq(constructor.prototype.values.name, "values");
    assertEq(constructor.prototype.values, constructor.prototype[Symbol.iterator]);

    assertDeepEq([...new constructor(0).values()], []);
    assertDeepEq([...new constructor(1).values()], [0]);
    assertDeepEq([...new constructor(2).values()], [0, 0]);
    assertDeepEq([...new constructor([15]).values()], [15]);

    var arr = new constructor([1, 2, 3]);
    var iterator = arr.values();
    assertDeepEq(iterator.next(), {value: 1, done: false});
    assertDeepEq(iterator.next(), {value: 2, done: false});
    assertDeepEq(iterator.next(), {value: 3, done: false});
    assertDeepEq(iterator.next(), {value: undefined, done: true});

    
    if (typeof newGlobal === "function") {
        var values = newGlobal()[constructor.name].prototype.values;
        assertDeepEq([...values.call(new constructor([42, 36]))], [42, 36]);
        arr = newGlobal()[constructor.name]([42, 36]);
        assertEq([...constructor.prototype.values.call(arr)].toString(), "42,36");
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.values.call(invalidReceiver);
        }, TypeError, "Assert that values fails if this value is not a TypedArray");
    });
    
    constructor.prototype.values.call(new Proxy(new constructor(), {}));
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
