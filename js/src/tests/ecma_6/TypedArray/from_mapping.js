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
    
    assertDeepEq(constructor.from([3, 4, 5], undefined), new constructor([3, 4, 5]));
    assertDeepEq(constructor.from([4, 5, 6], undefined, Math), new constructor([4, 5, 6]));

    
    var log = [];
    function f(...args) {
        log.push(args);
        return log.length;
    }
    assertDeepEq(constructor.from(['a', 'e', 'i', 'o', 'u'], f), new constructor([1, 2, 3, 4, 5]));
    assertDeepEq(log, [['a', 0], ['e', 1], ['i', 2], ['o', 3], ['u', 4]]);

    
    
    log = [];
    assertDeepEq(constructor.from({0: "zero", 1: "one", length: 2}, f), new constructor([1, 2]));
    assertDeepEq(log, [["zero", 0], ["one", 1]]);

    
    
    log = [];
    function C() {}
    C.from = constructor.from;
    var c = new C;
    c[0] = 1;
    c[1] = 2;
    assertDeepEq(C.from(["zero", "one"], f), c);
    assertDeepEq(log, [["zero", 0], ["one", 1]]);

    
    assertDeepEq(constructor.from([0, 1, , 3], String), new constructor(["0", "1", "undefined", "3"]));
    var arraylike = {length: 4, "0": 0, "1": 1, "3": 3};
    assertDeepEq(constructor.from(arraylike, String), new constructor(["0", "1", "undefined", "3"]));
}


assertDeepEq(Int8Array.from([150], v => v / 2), new Int8Array([75]));



if (typeof reportCompare === "function")
    reportCompare(true, true);
