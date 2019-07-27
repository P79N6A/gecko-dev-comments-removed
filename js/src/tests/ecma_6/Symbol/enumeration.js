


if (typeof Symbol === "function") {
    
    var obj = {};
    obj[Symbol.for("moon")] = "sun";
    obj[Symbol("asleep")] = "awake";
    obj[Symbol.iterator] = "List";
    for (var x in obj)
        throw "FAIL: " + uneval(x);

    
    var obj2 = Object.create(obj);
    for (var x in obj2)
        throw "FAIL: " + uneval(x);

    
    var p = new Proxy(obj, {});
    for (var x in p)
        throw "FAIL: " + uneval(x);
    var p2 = new Proxy(obj2, {});
    for (var x in p2)
        throw "FAIL: " + uneval(x);

    
    assertEq(Object.keys(obj).length, 0);
    assertEq(Object.keys(p).length, 0);
    assertEq(Object.keys(obj2).length, 0);
    assertEq(Object.keys(p2).length, 0);
    assertEq(Object.getOwnPropertyNames(obj).length, 0);
    assertEq(Object.getOwnPropertyNames(p).length, 0);
    assertEq(Object.getOwnPropertyNames(obj2).length, 0);
    assertEq(Object.getOwnPropertyNames(p2).length, 0);

    
    var log = [];
    var h = {
        ownKeys: (t) => {
            log.push("ownKeys");
            return ["a", "0", Symbol.for("moon"), Symbol("asleep"), Symbol.iterator];
        },
        getOwnPropertyDescriptor: (t, key) => {
            log.push("gopd", key);
            return {configurable: true, enumerable: true, value: 0, writable: true};
        }
    };
    p = new Proxy({}, h);
    assertDeepEq(Object.keys(p), ["a", "0"]);
    assertDeepEq(log, ["ownKeys", "gopd", "a", "gopd", "0"]);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
