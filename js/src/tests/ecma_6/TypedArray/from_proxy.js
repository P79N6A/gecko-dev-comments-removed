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
    
    var log = [];
    function LoggingProxy(target) {
        log.push("target", target);
        var h = {
            defineProperty: function (t, id) {
                log.push("define", id);
                return undefined;
            },
            has: function (t, id) {
                log.push("has", id);
                return id in t;
            },
            get: function (t, id) {
                log.push("get", id);
                return t[id];
            },
            set: function (t, id, v) {
                log.push("set", id);
                t[id] = v;
            }
        };
        return new Proxy(Object(target), h);
    }

    
    
    
    LoggingProxy.from = constructor.from;
    LoggingProxy.from([3, 4, 5]);
    assertDeepEq(log, ["target", 3, "set", "0", "set", "1", "set", "2"]);

    
    
    log = [];
    assertDeepEq(constructor.from(new LoggingProxy([3, 4, 5])), new constructor([3, 4, 5]));
    assertDeepEq(log, ["target", [3, 4, 5],
                       "get", Symbol.iterator,
                       "get", "length", "get", "0",
                       "get", "length", "get", "1",
                       "get", "length", "get", "2",
                       "get", "length"]);

    
    log = [];
    var arr = [5, 6, 7];
    arr[Symbol.iterator] = undefined;
    assertDeepEq(constructor.from(new LoggingProxy(arr)), new constructor([5, 6, 7]));
    assertDeepEq(log, ["target", [5, 6, 7],
                       "get", Symbol.iterator,
                       "get", "length",
                       "get", "0", "get", "1", "get", "2"]);
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
