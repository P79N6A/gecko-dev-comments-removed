



assertEq(typeof Reflect, 'object');
assertEq(Object.getPrototypeOf(Reflect), Object.prototype);
assertEq(Reflect.toString(), '[object Object]');
assertThrowsInstanceOf(() => new Reflect, TypeError);

var desc = Object.getOwnPropertyDescriptor(this, "Reflect");
assertEq(desc.enumerable, false);
assertEq(desc.configurable, true);
assertEq(desc.writable, true);

for (var name in Reflect)
    throw new Error("Reflect should not have any enumerable properties");


var methods = {
    apply: 3,
    construct: 2,
    defineProperty: 3,
    deleteProperty: 2,
    
    get: 2,
    getOwnPropertyDescriptor: 2,
    getPrototypeOf: 1,
    has: 2,
    isExtensible: 1,
    ownKeys: 1,
    preventExtensions: 1,
    set: 3,
    setPrototypeOf: 2
};


for (var name of Reflect.ownKeys(Reflect)) {
    
    
    if (name !== "parse")
        assertEq(name in methods, true, `unexpected property found: Reflect.${name}`);
}


for (var name of Object.keys(methods)) {
    var desc = Object.getOwnPropertyDescriptor(Reflect, name);
    assertEq(desc.enumerable, false);
    assertEq(desc.configurable, true);
    assertEq(desc.writable, true);
    var f = desc.value;
    assertEq(typeof f, "function");
    assertEq(f.length, methods[name]);
}



delete this.Reflect;
assertEq("Reflect" in this, false);

reportCompare(0, 0);
