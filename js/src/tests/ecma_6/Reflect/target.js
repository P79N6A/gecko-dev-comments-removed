






var methodInfo = {
    apply: [undefined, []],
    
    defineProperty: ["x", {}],
    deleteProperty: ["x"],
    
    get: ["x", {}],
    getOwnPropertyDescriptor: ["x"],
    getPrototypeOf: [],
    has: ["x"],
    isExtensible: [],
    ownKeys: [],
    preventExtensions: [],
    set: ["x", 0],
    setPrototypeOf: [{}]
};


for (var name of Reflect.ownKeys(Reflect)) {
    
    
    if (name !== "parse")
      assertEq(name in methodInfo, true);
}

for (var name of Object.keys(methodInfo)) {
    var args = methodInfo[name];

    
    assertThrowsInstanceOf(Reflect[name], TypeError);

    
    for (var value of SOME_PRIMITIVE_VALUES) {
        assertThrowsInstanceOf(() => Reflect[name](value, ...args), TypeError);
    }
}

reportCompare(0, 0);
