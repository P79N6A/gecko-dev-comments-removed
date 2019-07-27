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
    assertEq(constructor.prototype.reduce.length, 1);

    
    var arr = new constructor([1, 2, 3, 4, 5]);

    assertEq(arr.reduce((previous, current) => previous + current), 15);
    assertEq(arr.reduce((previous, current) => current - previous), 3);

    var count = 0;
    var sum = 0;
    assertEq(arr.reduce((previous, current, index, array) => {
        count++;
        sum += current;
        assertEq(current - 1, index);
        assertEq(current, arr[index]);
        assertEq(array, arr);
        return previous * current;
    }), 120);
    assertEq(count, 4);
    assertEq(sum, 14);

    
    assertEq(arr.reduce((previous, current) => previous + current, -15), 0);
    assertEq(arr.reduce((previous, current) => previous + current, ""), "12345");
    assertDeepEq(arr.reduce((previous, current) => previous.concat(current), []), [1, 2, 3, 4, 5]);

    
    var global = this;
    arr.reduce(function(){
        assertEq(this, global);
    });
    arr.reduce(function(){
        "use strict";
        assertEq(this, undefined);
    });
    arr.reduce(() => assertEq(this, global));

    
    var count = 0;
    var sum = 0;
    assertThrowsInstanceOf(() => {
        arr.reduce((previous, current, index, array) => {
            count++;
            sum += current;
            if (index === 3) {
                throw TypeError("reduce");
            }
        })
    }, TypeError);
    assertEq(count, 3);
    assertEq(sum, 9);

    
    assertThrowsInstanceOf(() => {
        arr.reduce();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.reduce(callback);
        }, TypeError);
    })

    
    arr.reduce(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var reduce = newGlobal()[constructor.name].prototype.reduce;
        assertEq(reduce.call(arr, (previous, current) => Math.min(previous, current)), 1);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.reduce.call(invalidReceiver, () => {});
        }, TypeError, "Assert that reduce fails if this value is not a TypedArray");
    });
    
    constructor.prototype.reduce.call(new Proxy(new constructor(3), {}), () => {});

    
    assertEq(Object.defineProperty(arr, "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).reduce((previous, current) => Math.max(previous, current)), 5);
}


for (var constructor of constructors) {
    assertEq(constructor.prototype.reduceRight.length, 1);

    
    var arr = new constructor([1, 2, 3, 4, 5]);

    assertEq(arr.reduceRight((previous, current) => previous + current), 15);
    assertEq(arr.reduceRight((previous, current) => current - previous), 3);

    var count = 0;
    var sum = 0;
    assertEq(arr.reduceRight((previous, current, index, array) => {
        count++;
        sum += current;
        assertEq(current - 1, index);
        assertEq(current, arr[index]);
        assertEq(array, arr);
        return previous * current;
    }), 120);
    assertEq(count, 4);
    assertEq(sum, 10);

    
    assertEq(arr.reduceRight((previous, current) => previous + current, -15), 0);
    assertEq(arr.reduceRight((previous, current) => previous + current, ""), "54321");
    assertDeepEq(arr.reduceRight((previous, current) => previous.concat(current), []), [5, 4, 3, 2, 1]);

    
    var global = this;
    arr.reduceRight(function(){
        assertEq(this, global);
    });
    arr.reduceRight(function(){
        "use strict";
        assertEq(this, undefined);
    });
    arr.reduceRight(() => assertEq(this, global));

    
    var count = 0;
    var sum = 0;
    assertThrowsInstanceOf(() => {
        arr.reduceRight((previous, current, index, array) => {
            count++;
            sum += current;
            if (index === 1) {
                throw TypeError("reduceRight");
            }
        })
    }, TypeError);
    assertEq(count, 3);
    assertEq(sum, 9);

    
    assertThrowsInstanceOf(() => {
        arr.reduceRight();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.reduceRight(callback);
        }, TypeError);
    })

    
    arr.reduceRight(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var reduceRight = newGlobal()[constructor.name].prototype.reduceRight;
        assertEq(reduceRight.call(arr, (previous, current) => Math.min(previous, current)), 1);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.reduceRight.call(invalidReceiver, () => {});
        }, TypeError, "Assert that reduceRight fails if this value is not a TypedArray");
    });
    
    constructor.prototype.reduceRight.call(new Proxy(new constructor(3), {}), () => {});

    
    assertEq(Object.defineProperty(arr, "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).reduceRight((previous, current) => Math.max(previous, current)), 5);
}

if (typeof reportCompare === "function")
    reportCompare(true, true);