






var log = [];
function LoggingProxy(target) {
    return new Proxy(target, {
        has: function (t, id) {
            log.push("has " + id);
            return id in t;
        },
        get: function (t, id) {
            log.push("get " + id);
            return t[id];
        }
    });
}



var testSubjects = [
    {},
    [],
    new Proxy({}, {})
];

for (var obj of testSubjects) {
    log = [];

    
    
    Object.defineProperty(obj, "x", new LoggingProxy({
        enumerable: true,
        configurable: true,
        value: 3,
        writable: true
    }));

    
    
    assertDeepEq(log, [
        "has enumerable", "get enumerable",
        "has configurable", "get configurable",
        "has value", "get value",
        "has writable", "get writable",
        "has get",
        "has set"
    ]);
}

reportCompare(0, 0);
