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
    assertEq(constructor.prototype.map.length, 1);

    
    assertDeepEq(new constructor([1, 3, 5]).map(v => v * 2), new constructor([2,6,10]));
    assertDeepEq(new constructor([-1, 13, 5]).map(v => v - 2), new constructor([-3, 11, 3]));
    assertDeepEq(new constructor(10).map(v => v), new constructor(10));
    assertDeepEq(new constructor().map(v => v + 1), new constructor);
    assertDeepEq(new constructor([1,2,3]).map(v => v), new constructor([1,2,3]));

    var arr = new constructor([1, 2, 3, 4, 5]);
    var sum = 0;
    var count = 0;
    assertDeepEq(arr.map((v, k, o) => {
        count++;
        sum += v;
        assertEq(k, v - 1);
        assertEq(o, arr);
        return v;
    }), arr);
    assertEq(sum, 15);
    assertEq(count, 5);

    
    var changeArr = new constructor([1,2,3,4,5]);
    assertDeepEq(arr.map((v,k) => {
        changeArr[k] = v + 1;
        return v;
    }), new constructor([1,2,3,4,5]));

    
    function assertThisArg(thisArg, thisValue) {
        
        assertDeepEq(arr.map(function(v) {
            assertDeepEq(this, thisValue);
            return v;
        }, thisArg), arr);

        
        assertDeepEq(arr.map(function(v) {
            "use strict";
            assertDeepEq(this, thisArg);
            return v;
        }, thisArg), arr);

        
        var self = this;
        assertDeepEq(arr.map((v) => {
            assertEq(this, self);
            return v;
        }, thisArg), arr);
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
        arr.map((v, k, o) => {
            count++;
            sum += v;
            assertEq(k, v - 1);
            assertEq(o, arr);
            if (v === 3) {
                throw "map";
            }
            return v;
        })
    } catch(e) {
        assertEq(e, "map");
        thrown = true;
    }
    assertEq(thrown, true);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    assertThrowsInstanceOf(() => {
        arr.map();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.map(callback);
        }, TypeError);
    })

    
    arr.map(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var map = newGlobal()[constructor.name].prototype.map;
        var sum = 0;
        assertDeepEq(map.call(new constructor([1, 2, 3]), v => sum += v), new constructor([1,3,6]));
        assertEq(sum, 6);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
                            new Proxy(new constructor(), {})];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.filter.call(invalidReceiver, () => true);
        }, TypeError, "Assert that map fails if this value is not a TypedArray");
    });

    
    assertDeepEq(Object.defineProperty(new constructor([1, 2, 3]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).map((b) => b), new constructor([1,2,3]));
}


for (var constructor of constructors) {
    assertEq(constructor.prototype.filter.length, 1)

    
    assertDeepEq(new constructor([1,2,3]).filter(x => x == x), new constructor([1,2,3]));
    assertDeepEq(new constructor([1,2,3,4]).filter(x => x % 2 == 0), new constructor([2,4]));
    assertDeepEq(new constructor([1,2,3,4,5]).filter(x => x < 4), new constructor([1,2,3]));
    assertDeepEq(new constructor().filter(x => x * 2 == 4), new constructor());

    var arr = new constructor([1,2,3,4,5]);
    var sum = 0;
    var count = 0;
    assertDeepEq(arr.filter((v, k, o) => {
        count++;
        sum += v;
        assertEq(k, v - 1);
        assertEq(o, arr);
        return (v < 4);
    }), new constructor([1,2,3]));
    assertEq(sum, 15);
    assertEq(count, 5);

    
    var changeArr = new constructor([1,2,3,4,5]);
    assertDeepEq(arr.filter((v,k) => {
        changeArr[k] = v + 1;
        return true;
    }), new constructor([1,2,3,4,5]));

    
    function assertThisArg(thisArg, thisValue) {
        
        assertDeepEq(arr.filter(function(v) {
            assertDeepEq(this, thisValue);
            return v;
        }, thisArg), arr);

        
        assertDeepEq(arr.filter(function(v) {
            "use strict";
            assertDeepEq(this, thisArg);
            return v;
        }, thisArg), arr);

        
        var self = this;
        assertDeepEq(arr.filter((v) => {
            assertEq(this, self);
            return v;
        }, thisArg), arr);
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
        arr.filter((v, k, o) => {
            count++;
            sum += v;
            assertEq(k, v - 1);
            assertEq(o, arr);
            if (v === 3) {
                throw "filter";
            }
            return v;
        })
    } catch(e) {
        assertEq(e, "filter");
        thrown = true;
    }
    assertEq(thrown, true);
    assertEq(sum, 6);
    assertEq(count, 3);

    
    assertThrowsInstanceOf(() => {
        arr.filter();
    }, TypeError);
    var invalidCallbacks = [undefined, null, 1, false, "", Symbol(), [], {}, /./];
    invalidCallbacks.forEach(callback => {
        assertThrowsInstanceOf(() => {
            arr.filter(callback);
        }, TypeError);
    })

    
    arr.filter(function*(){
        throw "This line will not be executed";
    });

    
    if (typeof newGlobal === "function") {
        var filter = newGlobal()[constructor.name].prototype.filter;
        var sum = 0;
        assertDeepEq(filter.call(new constructor([1, 2, 3]), v => {sum += v; return true}),
        new constructor([1,2,3]));
        assertEq(sum, 6);
    }

    
    var invalidReceivers = [undefined, null, 1, false, "", Symbol(), [], {}, /./,
                            new Proxy(new constructor(), {})];
    invalidReceivers.forEach(invalidReceiver => {
        assertThrowsInstanceOf(() => {
            constructor.prototype.filter.call(invalidReceiver, () => true);
        }, TypeError, "Assert that filter fails if this value is not a TypedArray");
    });

    
    assertDeepEq(Object.defineProperty(new constructor([1, 2, 3]), "length", {
        get() {
            throw new Error("length accessor called");
        }
    }).filter((b) => true), new constructor([1,2,3]));
}






var obj = {
    v: 0,
    next: function() {
        if (this.v == 5) {
	       return {done : true, value : this.v };
        } else {
	       this.v++;
	       return { done : false, value : this.v };
        }
    }
};


var old = Array.prototype[Symbol.iterator];

Array.prototype[Symbol.iterator] = obj;
assertDeepEq(new Uint16Array([1,2,3]).filter(v => true), new Uint16Array([1,2,3]));


Array.prototype[Symbol.iterator] = old;




Object.defineProperty(Array.prototype, 0, {configurable: true, get: function() { return 1; }, set: function() { this.b = 1; }});
assertDeepEq(new Uint16Array([1,2,3]).filter(v => true), new Uint16Array([1,2,3]));
delete Array.prototype[0];

if (typeof reportCompare === "function")
    reportCompare(true, true);
