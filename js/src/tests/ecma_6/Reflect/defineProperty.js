



var obj = {};
assertEq(Reflect.defineProperty(obj, "x", {value: 7}), true);
assertEq(obj.x, 7);
var desc = Reflect.getOwnPropertyDescriptor(obj, "x");
assertDeepEq(desc, {value: 7,
                    writable: false,
                    enumerable: false,
                    configurable: false});


var key = Symbol(":o)");
assertEq(Reflect.defineProperty(obj, key, {value: 8}), true);
assertEq(obj[key], 8);


obj = [1, 2, 3, 4, 5];
assertEq(Reflect.defineProperty(obj, "length", {value: 4}), true);
assertDeepEq(obj, [1, 2, 3, 4]);


obj = {};
var proxy = new Proxy(obj, {
    defineProperty(t, id, desc) {
        t[id] = 1;
        return true;
    }
});
assertEq(Reflect.defineProperty(proxy, "prop", {value: 7}), true);
assertEq(obj.prop, 1);
assertEq(delete obj.prop, true);
assertEq("prop" in obj, false);



obj = {};
var attributes = {
    configurable: 17,
    enumerable: undefined,
    value: null
};
proxy = new Proxy(obj, {
    defineProperty(t, id, desc) {
        assertEq(desc !== attributes, true);
        assertEq(desc.configurable, true);
        assertEq(desc.enumerable, false);
        assertEq(desc.value, null);
        assertEq("writable" in desc, false);
        return 15;  
    }
});
assertEq(Reflect.defineProperty(proxy, "prop", attributes), true);














assertThrowsInstanceOf(() => Reflect.defineProperty(obj, "y"),
                       TypeError);


for (var attributes of SOME_PRIMITIVE_VALUES) {
    assertThrowsInstanceOf(() => Reflect.defineProperty(obj, "y", attributes),
                           TypeError);
}


obj = Object.preventExtensions({});
assertEq(Reflect.defineProperty(obj, "prop", {value: 4}), false);


obj = Object.preventExtensions(Object.create({"prop": 3}));
assertEq(Reflect.defineProperty(obj, "prop", {value: 4}), false);


obj = Object.freeze({prop: 1});
assertEq(Reflect.defineProperty(obj, "prop", {configurable: true}), false);


obj = Object.freeze(Object.defineProperties({}, {
    x: {enumerable: true,  configurable: false, value: 0},
    y: {enumerable: false, configurable: false, value: 0},
}));
assertEq(Reflect.defineProperty(obj, "x", {enumerable: false}), false);
assertEq(Reflect.defineProperty(obj, "y", {enumerable: true}), false);


obj = Object.seal({x: 1, get y() { return 2; }});
assertEq(Reflect.defineProperty(obj, "x", {get() { return 2; }}), false);
assertEq(Reflect.defineProperty(obj, "y", {value: 1}), false);


obj = Object.freeze({prop: 0});
assertEq(Reflect.defineProperty(obj, "prop", {writable: true}), false);
assertEq(Reflect.defineProperty(obj, "prop", {writable: false}), true);  


obj = Object.freeze({prop: 0});
assertEq(Reflect.defineProperty(obj, "prop", {value: -0}), false);
assertEq(Reflect.defineProperty(obj, "prop", {value: +0}), true);  


function g() {}
function s(x) {}
obj = {};
Object.defineProperty(obj, "prop", {get: g, set: s, configurable: false});
assertEq(Reflect.defineProperty(obj, "prop", {get: s}), false);
assertEq(Reflect.defineProperty(obj, "prop", {get: g}), true);  
assertEq(Reflect.defineProperty(obj, "prop", {set: g}), false);
assertEq(Reflect.defineProperty(obj, "prop", {set: s}), true);  


var falseValues = [false, 0, -0, "", NaN, null, undefined];
if (typeof objectEmulatingUndefined === "function")
    falseValues.push(objectEmulatingUndefined());
var value;
proxy = new Proxy({}, {
    defineProperty(t, id, desc) {
        return value;
    }
});
for (value of falseValues) {
    assertEq(Reflect.defineProperty(proxy, "prop", {value: 1}), false);
}



obj = Object.freeze({x: 1});
proxy = new Proxy(obj, {
    defineProperty(t, id, desc) {
        return true;
    }
});
assertThrowsInstanceOf(() => Reflect.defineProperty(proxy, "x", {value: 2}), TypeError);
assertThrowsInstanceOf(() => Reflect.defineProperty(proxy, "y", {value: 0}), TypeError);
assertEq(Reflect.defineProperty(proxy, "x", {value: 1}), true);



var poison =
  (counter => new Proxy({}, new Proxy({}, { get() { throw counter++; } })))(42);
assertThrowsValue(() => {
    Reflect.defineProperty(poison, {
        toString() { throw 17; },
        valueOf() { throw 8675309; }
    }, poison);
}, 17);




reportCompare(0, 0);
