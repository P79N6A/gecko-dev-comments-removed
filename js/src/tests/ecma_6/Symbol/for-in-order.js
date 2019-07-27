


if (typeof Symbol === "function") {
    
    
    

    var obj = {};
    obj[Symbol("moon")] = 0;
    obj.x = 1;
    obj[Symbol.for("y")] = 2
    obj.y = 3;
    obj[Symbol.iterator] = function* () { yield 4; };
    obj.z = 5;
    Object.prototype[Symbol.for("comet")] = 6;

    var keys = [];
    for (var k in obj)
        keys.push(k);
    assertDeepEq(keys, ["x", "y", "z"]);
    assertDeepEq(Object.keys(obj), ["x", "y", "z"]);

    
    for (var i = 0; i < 1000; i++)
        obj[Symbol(i)] = i;
    obj.w = 1000;
    keys = []
    for (var k in obj)
        keys.push(k);
    assertDeepEq(keys, ["x", "y", "z", "w"]);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
