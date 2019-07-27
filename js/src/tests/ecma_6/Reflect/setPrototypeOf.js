



var obj = {};
assertEq(Object.getPrototypeOf(obj), Object.prototype);
var proto = {};
assertEq(Reflect.setPrototypeOf(obj, proto), true);
assertEq(Object.getPrototypeOf(obj), proto);


obj = {};
assertEq(Reflect.setPrototypeOf(obj, null), true);
assertEq(Object.getPrototypeOf(obj), null);


obj = {};
assertThrowsInstanceOf(() => Reflect.setPrototypeOf(obj), TypeError);


for (proto of [undefined, false, 0, 1.6, "that", Symbol.iterator]) {
    assertThrowsInstanceOf(() => Reflect.setPrototypeOf(obj, proto), TypeError);
}


proto = {};
obj = Object.preventExtensions(Object.create(proto));
assertEq(Reflect.setPrototypeOf(obj, {}), false);
assertEq(Reflect.setPrototypeOf(obj, null), false);
assertEq(Reflect.setPrototypeOf(obj, proto), false);  


obj = {};
assertEq(Reflect.setPrototypeOf(obj, obj), false);


obj = Object.create(proto);
assertEq(Reflect.setPrototypeOf(proto, obj), false);


for (var i = 0; i < 256; i++)
    obj = Object.create(obj);
assertEq(Reflect.setPrototypeOf(proto, obj), false);



obj = {};
var proxy = new Proxy(Object.create(obj), {});
if (Reflect.setPrototypeOf(obj, proxy) !== false) {
    throw new Error("Congratulations on implementing ES6 [[SetPrototype]]! " +
                    "Update this test for 1 karma point!");
    
}





var hits = 0;
proto = {name: "proto"};
obj = {name: "obj"};
proxy = new Proxy(obj, {
    setPrototypeOf(t, p) {
        assertEq(t, obj);
        assertEq(p, proto);
        hits++;
        return 0;
    }
});
if (Reflect.setPrototypeOf(proxy, proto) !== true) {
    throw new Error("Congratulations on implementing the setPrototypeOf trap for proxies! " +
                    "Please update this test.");
    
    
}





reportCompare(0, 0);
