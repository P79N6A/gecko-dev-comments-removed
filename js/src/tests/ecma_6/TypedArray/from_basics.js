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
    
    assertEq(constructors[0].from === constructor.from, true);

    
    var src = new constructor([1, 2, 3]), copy = constructor.from(src);
    assertEq(copy === src, false);
    assertEq(copy instanceof constructor, true);
    assertDeepEq(copy, src);

    
    var a = new constructor([0, 1]);
    a.name = "lisa";
    assertDeepEq(constructor.from(a), new constructor([0, 1]));

    
    src = {0: 0, 1: 1, length: 2};
    copy = constructor.from(src);
    assertEq(copy instanceof constructor, true);
    assertDeepEq(copy, new constructor([0, 1]));

    
    src = {0: "0", 1: "1", 2: "two", 9: "nine", name: "lisa", length: 2};
    assertDeepEq(constructor.from(src), new constructor([0, 1]));

    
    
    assertDeepEq(constructor.from({}), new constructor());

    
    assertDeepEq(constructor.from(1), new constructor());
    assertDeepEq(constructor.from("123"), new constructor([1, 2, 3]));
    assertDeepEq(constructor.from(true), new constructor());
    assertDeepEq(constructor.from(Symbol()), new constructor());

    
    src = {length: 2, 1: "1", 0: "0"};
    assertDeepEq(constructor.from(src), new constructor([0, 1]));
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
