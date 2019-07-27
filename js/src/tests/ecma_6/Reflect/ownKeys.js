







var sym = Symbol.for("comet");
var sym2 = Symbol.for("meteor");
var cases = [
    {object: {z: 3, y: 2, x: 1},
     keys: ["z", "y", "x"]},
    {object: [],
     keys: ["length"]},
    {object: new Int8Array(4),
     keys: ["0", "1", "2", "3"]},
    {object: new Proxy({a: 7}, {}),
     keys: ["a"]},
    {object: {[sym]: "ok"},
     keys: [sym]},
    {object: {[sym]: 0,  
              "str": 0,
              "773": 0,
              "0": 0,
              [sym2]: 0,
              "-1": 0,
              "8": 0,
              "second str": 0},
     keys: ["0", "8", "773",  
            "str", "-1", "second str", 
            sym, sym2]}, 
    {object: newGlobal().Math,  
     keys: Reflect.ownKeys(Math)}
];
for (var {object, keys} of cases)
    assertDeepEq(Reflect.ownKeys(object), keys);


var object = {}, keys = [];
for (var i = 0; i < 3; i++) {
    var newKeys = Reflect.ownKeys(object);
    assertEq(newKeys !== keys, true);
    keys = newKeys;
}


keys = ["str", "0"];
obj = {};
proxy = new Proxy(obj, {
    ownKeys() { return keys; }
});
var actual = Reflect.ownKeys(proxy);
assertDeepEq(actual, keys);  
assertEq(actual !== keys, true);  


var obj = Object.preventExtensions({});
var proxy = new Proxy(obj, {
    ownKeys() { return ["something"]; }
});
assertThrowsInstanceOf(() => Reflect.ownKeys(proxy), TypeError);



reportCompare(0, 0);
