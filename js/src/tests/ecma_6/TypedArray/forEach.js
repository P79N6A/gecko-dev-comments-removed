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
    assertEq(constructor.prototype.forEach.length, 1);

    var arr = new constructor([1, 2, 3, 4, 5]);
    
    function assertThisArg(thisArg, thisValue) {
        
        arr.forEach(function() {
            assertDeepEq(this, thisValue);
            return false;
        }, thisArg);

        
        arr.forEach(function() {
            "use strict";
            assertDeepEq(this, thisArg);
            return false;
        }, thisArg);

        
        var self = this;
        arr.forEach(() => {
            assertEq(this, self);
            return false;
        }, thisArg);
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
        assertEq(arr.forEach((v) => {
            count++;
            sum += v;
            if (v === 3) {
                throw "forEach";
            }
        }), undefined)
    } catch(e) {
        assertEq(e, "forEach");
        thrown = true;
    }
    assertEq(thrown, true);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    assertThrowsInstanceOf(() => {
        arr.forEach();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.forEach(callback);
        }, TypeError);
    })

    
    arr.forEach(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var forEach = newGlobal()[constructor.name].prototype.forEach;
        var sum = 0;
        forEach.call(new constructor([1, 2, 3]), v => {
            sum += v;
        });
        assertEq(sum, 6);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
			    new Proxy(new constructor(), {})];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.forEach.call(invalidReceiver, () => true);
        }, TypeError, "Assert that some fails if this value is not a TypedArray");
    });
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
