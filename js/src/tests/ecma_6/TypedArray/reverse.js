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
    assertDeepEq(constructor.prototype.reverse.length, 0);

    assertDeepEq(new constructor().reverse(), new constructor());
    assertDeepEq(new constructor(10).reverse(), new constructor(10));
    assertDeepEq(new constructor([]).reverse(), new constructor([]));
    assertDeepEq(new constructor([1]).reverse(), new constructor([1]));
    assertDeepEq(new constructor([1, 2]).reverse(), new constructor([2, 1]));
    assertDeepEq(new constructor([1, 2, 3]).reverse(), new constructor([3, 2, 1]));
    assertDeepEq(new constructor([1, 2, 3, 4]).reverse(), new constructor([4, 3, 2, 1]));
    assertDeepEq(new constructor([1, 2, 3, 4, 5]).reverse(), new constructor([5, 4, 3, 2, 1]));
    assertDeepEq(new constructor([.1, .2, .3]).reverse(), new constructor([.3, .2, .1]));

    
    if (typeof newGlobal === "function") {
        var reverse = newGlobal()[constructor.name].prototype.reverse;
        assertDeepEq(reverse.call(new constructor([3, 2, 1])), new constructor([1, 2, 3]));
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.reverse.call(invalidReceiver);
        }, TypeError, "Assert that reverse fails if this value is not a TypedArray");
    });
    
    constructor.prototype.reverse.call(new Proxy(new constructor(), {}));

    
    Object.defineProperty(new constructor([1, 2, 3]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).reverse();
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
