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
    assertEq(constructor.prototype.entries.length, 0);
    assertEq(constructor.prototype.entries.name, "entries");

    assertDeepEq([...new constructor(0).entries()], []);
    assertDeepEq([...new constructor(1).entries()], [[0, 0]]);
    assertDeepEq([...new constructor(2).entries()], [[0, 0], [1, 0]]);
    assertDeepEq([...new constructor([15]).entries()], [[0, 15]]);

    var arr = new constructor([1, 2, 3]);
    var iterator = arr.entries();
    assertDeepEq(iterator.next(), {value: [0, 1], done: false});
    assertDeepEq(iterator.next(), {value: [1, 2], done: false});
    assertDeepEq(iterator.next(), {value: [2, 3], done: false});
    assertDeepEq(iterator.next(), {value: undefined, done: true});

    
    if (typeof newGlobal === "function") {
        var entries = newGlobal()[constructor.name].prototype.entries;
        assertDeepEq([...entries.call(new constructor(2))], [[0, 0], [1, 0]]);
        arr = newGlobal()[constructor.name](2);
        assertEq([...constructor.prototype.entries.call(arr)].toString(), "0,0,1,0");
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.entries.call(invalidReceiver);
        }, TypeError, "Assert that entries fails if this value is not a TypedArray");
    });
    
    constructor.prototype.entries.call(new Proxy(new constructor(), {}));
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
