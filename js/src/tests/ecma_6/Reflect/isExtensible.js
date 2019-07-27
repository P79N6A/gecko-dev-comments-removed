







var someObjects = [
    {},
    {a: "a"},
    [0, 1],
    new Uint8Array(64),
    Object(Symbol("table")),
    new Proxy({}, {})
];
for (var obj of someObjects) {
    assertEq(Reflect.isExtensible(obj), true);
    assertEq(Reflect.preventExtensions(obj), true);
    assertEq(Reflect.isExtensible(obj), false);
}


var arr = [0, 1, 2, 3];
Object.defineProperty(arr, "length", {writable: false});
assertEq(Reflect.isExtensible(arr), true);


for (var ext of [true, false]) {
    var obj = {};
    if (!ext)
        Object.preventExtensions(obj);
    var proxy = new Proxy(obj, {
        isExtensible() { return ext; }
    });
    assertEq(Reflect.isExtensible(proxy), ext);
}


proxy = new Proxy({}, {
    isExtensible() { throw "oops"; }
});
assertThrowsValue(() => Reflect.isExtensible(proxy), "oops");



proxy = new Proxy({}, {
    isExtensible() { return false; }
});
assertThrowsInstanceOf(() => Reflect.isExtensible(proxy), TypeError);



reportCompare(0, 0);
