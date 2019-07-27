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
    assertEq(constructor.prototype.every.length, 1);

    
    assertEq(new constructor([1, 3, 5]).every(v => v % 2), true);
    assertEq(new constructor([1, 3, 5]).every(v => v > 2), false);
    assertEq(new constructor(10).every(v => v === 0), true);
    assertEq(new constructor().every(v => v > 1), true);

    var arr = new constructor([1, 2, 3, 4, 5]);
    var sum = 0;
    var count = 0;
    assertEq(arr.every((v, k, o) => {
        count++;
        sum += v;
        assertEq(k, v - 1);
        assertEq(o, arr);
        return v < 3;
    }), false);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    function assertThisArg(thisArg, thisValue) {
        
        assertEq(arr.every(function() {
            assertDeepEq(this, thisValue);
            return true;
        }, thisArg), true);

        
        assertEq(arr.every(function() {
            "use strict";
            assertDeepEq(this, thisArg);
            return true;
        }, thisArg), true);

        
        var self = this;
        assertEq(arr.every(() => {
            assertEq(this, self);
            return true;
        }, thisArg), true);
    }
    assertThisArg([1, 2, 3], [1, 2, 3]);
    assertThisArg(Object, Object);
    assertThisArg(1, Object(1));
    assertThisArg("1", Object("1"));
    assertThisArg(false, Object(false));
    assertThisArg(undefined, this);
    assertThisArg(null, this);

    
    var sum = 0;
    var count = 0;
    var thrown = false;
    try {
        arr.every((v, k, o) => {
            count++;
            sum += v;
            assertEq(k, v - 1);
            assertEq(o, arr);
            if (v === 3) {
                throw "every";
            }
            return true
        })
    } catch(e) {
        assertEq(e, "every");
        thrown = true;
    }
    assertEq(thrown, true);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    assertThrowsInstanceOf(() => {
        arr.every();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.every(callback);
        }, TypeError);
    })

    
    arr.every(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var every = newGlobal()[constructor.name].prototype.every;
        var sum = 0;
        assertEq(every.call(new constructor([1, 2, 3]), v => sum += v), true);
        assertEq(sum, 6);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
                            new Proxy(new constructor(), {})];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.every.call(invalidReceiver, () => true);
        }, TypeError, "Assert that every fails if this value is not a TypedArray");
    });

    
    assertEq(Object.defineProperty(new constructor([1, 2, 3]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).every(() => true), true);
}

assertEq(new Float32Array([undefined, , NaN]).every(v => Object.is(v, NaN)), true);
assertEq(new Float64Array([undefined, , NaN]).every(v => Object.is(v, NaN)), true);


for (var constructor of constructors) {
    assertEq(constructor.prototype.some.length, 1);

    
    assertEq(new constructor([1, 2, 3]).some(v => v % 2), true);
    assertEq(new constructor([0, 2, 4]).some(v => v % 2), false);
    assertEq(new constructor([1, 3, 5]).some(v => v > 2), true);
    assertEq(new constructor([1, 3, 5]).some(v => v < 0), false);
    assertEq(new constructor(10).some(v => v !== 0), false);
    assertEq(new constructor().some(v => v > 1), false);

    var arr = new constructor([1, 2, 3, 4, 5]);
    var sum = 0;
    var count = 0;
    assertEq(arr.some((v, k, o) => {
        count++;
        sum += v;
        assertEq(k, v - 1);
        assertEq(o, arr);
        return v > 2;
    }), true);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    function assertThisArg(thisArg, thisValue) {
        
        assertEq(arr.some(function() {
            assertDeepEq(this, thisValue);
            return false;
        }, thisArg), false);

        
        assertEq(arr.some(function() {
            "use strict";
            assertDeepEq(this, thisArg);
            return false;
        }, thisArg), false);

        
        var self = this;
        assertEq(arr.some(() => {
            assertEq(this, self);
            return false;
        }, thisArg), false);
    }
    assertThisArg([1, 2, 3], [1, 2, 3]);
    assertThisArg(Object, Object);
    assertThisArg(1, Object(1));
    assertThisArg("1", Object("1"));
    assertThisArg(false, Object(false));
    assertThisArg(undefined, this);
    assertThisArg(null, this);

    
    var sum = 0;
    var count = 0;
    var thrown = false;
    try {
        arr.some((v, k, o) => {
            count++;
            sum += v;
            assertEq(k, v - 1);
            assertEq(o, arr);
            if (v === 3) {
                throw "some";
            }
            return false
        })
    } catch(e) {
        assertEq(e, "some");
        thrown = true;
    }
    assertEq(thrown, true);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    assertThrowsInstanceOf(() => {
        arr.some();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.some(callback);
        }, TypeError);
    })

    
    arr.some(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var some = newGlobal()[constructor.name].prototype.some;
        var sum = 0;
        assertEq(some.call(new constructor([1, 2, 3]), v => {
            sum += v;
            return false;
        }), false);
        assertEq(sum, 6);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
                            new Proxy(new constructor(), {})];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.some.call(invalidReceiver, () => true);
        }, TypeError, "Assert that some fails if this value is not a TypedArray");
    });

    
    assertEq(Object.defineProperty(new constructor([1, 2, 3]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).some(() => false), false);
}

assertEq(new Float32Array([undefined, , NaN]).some(v => v === v), false);
assertEq(new Float64Array([undefined, , NaN]).some(v => v === v), false);

if (typeof reportCompare === "function")
    reportCompare(true, true);
