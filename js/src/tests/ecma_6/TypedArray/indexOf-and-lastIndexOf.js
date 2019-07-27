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

    assertEq(constructor.prototype.indexOf.length, 1);

    
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(0), -1);
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(1), 0);
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(5), 4);
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(6), -1);
    assertEq(new constructor([1, 2, 1, 2, 1]).indexOf(1), 0);

    if (constructor === Float32Array || constructor === Float64Array) {
        assertEq(new constructor([NaN, 0, -0]).indexOf(NaN), -1);
        assertEq(new constructor([NaN, 0, -0]).indexOf(0), 1);
        assertEq(new constructor([NaN, 0, -0]).indexOf(-0), 1);
    } else {
        
        assertEq(new constructor([NaN, 0, -0]).indexOf(NaN), -1);
        assertEq(new constructor([NaN, 0, -0]).indexOf(0), 0);
        assertEq(new constructor([NaN, 0, -0]).indexOf(-0), 0);
    }

    
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(1, 1), -1);
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(1, -100), 0);
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(3, 100), -1);
    assertEq(new constructor([1, 2, 3, 4, 5]).indexOf(5, -1), 4);
    assertEq(new constructor([1, 2, 1, 2, 1]).indexOf(1, 2), 2);
    assertEq(new constructor([1, 2, 1, 2, 1]).indexOf(1, -2), 4);

    
    var nonTypedArrays = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
                         
                         ];
    nonTypedArrays.forEach(nonTypedArray => {
        assertThrowsInstanceOf(function() {
            constructor.prototype.indexOf.call(nonTypedArray);
        }, TypeError, "Assert that indexOf fails if this value is not a TypedArray");
    });

    
    assertEq(Object.defineProperty(new constructor([0, 1, 2, 3, 5]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).indexOf(1), 1);
}

assertEq(new Float32Array([.1, .2, .3]).indexOf(.2), -1);
assertEq(new Float32Array([.1, .2, .3]).indexOf(Math.fround(.2)), 1);
assertEq(new Float64Array([.1, .2, .3]).indexOf(.2), 1);


for (var constructor of constructors) {

    assertEq(constructor.prototype.lastIndexOf.length, 1);

    
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(0), -1);
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(1), 0);
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(5), 4);
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(6), -1);
    assertEq(new constructor([1, 2, 1, 2, 1]).lastIndexOf(1), 4);

    if (constructor === Float32Array || constructor === Float64Array) {
        assertEq(new constructor([NaN, 0, -0]).lastIndexOf(NaN), -1);
        assertEq(new constructor([NaN, 0, -0]).lastIndexOf(0), 2);
        assertEq(new constructor([NaN, 0, -0]).lastIndexOf(-0), 2);
    } else {
        
        assertEq(new constructor([NaN, 0, -0]).lastIndexOf(NaN), -1);
        assertEq(new constructor([NaN, 0, -0]).lastIndexOf(0), 2);
        assertEq(new constructor([NaN, 0, -0]).lastIndexOf(-0), 2);
    }

    
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(1, 1), 0);
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(1, -100), -1);
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(3, 100), 2);
    assertEq(new constructor([1, 2, 3, 4, 5]).lastIndexOf(5, -1), 4);
    assertEq(new constructor([1, 2, 1, 2, 1]).lastIndexOf(1, 2), 2);
    assertEq(new constructor([1, 2, 1, 2, 1]).lastIndexOf(1, -2), 2);

    
    var nonTypedArrays = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
                         
                         ];
    nonTypedArrays.forEach(nonTypedArray => {
        assertThrowsInstanceOf(function() {
            constructor.prototype.lastIndexOf.call(nonTypedArray);
        }, TypeError, "Assert that lastIndexOf fails if this value is not a TypedArray");
    });

    
    assertEq(Object.defineProperty(new constructor([0, 1, 2, 3, 5]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).lastIndexOf(1), 1);
}

assertEq(new Float32Array([.1, .2, .3]).lastIndexOf(.2), -1);
assertEq(new Float32Array([.1, .2, .3]).lastIndexOf(Math.fround(.2)), 1);
assertEq(new Float64Array([.1, .2, .3]).lastIndexOf(.2), 1);

if (typeof reportCompare === "function")
    reportCompare(true, true);
