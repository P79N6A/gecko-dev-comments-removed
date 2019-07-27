


if (typeof Symbol === "function") {
    

    for (var T of [Uint8Array, Uint8ClampedArray, Int16Array, Float32Array]) {
        
        assertThrowsInstanceOf(() => new T([Symbol("a")]), TypeError);

        
        var arr = new T([1]);
        assertThrowsInstanceOf(() => { arr[0] = Symbol.iterator; }, TypeError);
        assertEq(arr[0], 1);
    }
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
