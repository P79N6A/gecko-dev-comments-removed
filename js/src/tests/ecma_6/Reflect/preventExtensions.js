





var someObjects = [
    {},
    new Int32Array(7),
    Object(Symbol("table")),
    new Proxy({}, {})
];

for (var obj of someObjects) {
    assertEq(Reflect.preventExtensions(obj), true);
    
    assertEq(Reflect.preventExtensions(obj), true);
}


assertThrowsInstanceOf(() => Reflect.isExtensible(), TypeError);
for (var value of [undefined, null, true, 1, NaN, "Phaedo", Symbol("good")]) {
    assertThrowsInstanceOf(() => Reflect.isExtensible(value), TypeError);
}


obj = {};
var proxy = new Proxy(obj, {
    preventExtensions() { return false; }
});
assertEq(Reflect.preventExtensions(proxy), false);
assertEq(Reflect.isExtensible(obj), true);
assertEq(Reflect.isExtensible(proxy), true);


obj = {};
proxy = new Proxy(obj, {
    preventExtensions() { throw "fit"; }
});
assertThrowsValue(() => Reflect.preventExtensions(proxy), "fit");
assertEq(Reflect.isExtensible(obj), true);
assertEq(Reflect.isExtensible(proxy), true);



obj = {};
proxy = new Proxy(obj, {
    preventExtensions() { return true; }
});
assertThrowsInstanceOf(() => Reflect.preventExtensions(proxy), TypeError);
assertEq(Reflect.isExtensible(obj), true);
assertEq(Reflect.isExtensible(proxy), true);



reportCompare(0, 0);
