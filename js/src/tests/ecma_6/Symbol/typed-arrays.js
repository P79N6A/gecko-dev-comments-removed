




var tests = [
    {T: Uint8Array, result: 0},
    {T: Uint8ClampedArray, result: 0},
    {T: Int16Array, result: 0},
    {T: Float32Array, result: NaN}
];

for (var {T, result} of tests) {
    
    var arr = new T([Symbol("a")]);
    assertEq(arr.length, 1);
    assertEq(arr[0], result);

    
    arr[0] = 0;
    assertEq(arr[0] = Symbol.iterator, Symbol.iterator);
    assertEq(arr[0], result);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
