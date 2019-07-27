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
    
    
    var hits = 0, obj = {};
    function f(x) {
        assertEq(this, obj);
        hits++;
    }
    constructor.from(["a", "b", "c"], f, obj);
    assertEq(hits, 3);

    
    hits = 0;
    function gs(x) {
        "use strict";
        assertEq(this, undefined);
        hits++;
    }
    constructor.from("def", gs);
    assertEq(hits, 3);

    
    
    var global = this;
    hits = 0;
    function g(x) {
        assertEq(this, global);
        hits++;
    }
    constructor.from("ghi", g);
    assertEq(hits, 3);

    
    for (var v of [0, "str", undefined]) {
        hits = 0;
        var mapfn = function h(x) {
            "use strict";
            assertEq(this, v);
            hits++;
        };
        constructor.from("pq", mapfn, v);
        assertEq(hits, 2);
    }

    
    
    for (var v of [0, "str", true]) {
        hits = 0;
        var mapfn = function h(x) {
            assertDeepEq(this, Object(v));
            hits++;
        };
        constructor.from("pq", mapfn, v);
        assertEq(hits, 2);
    }
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
