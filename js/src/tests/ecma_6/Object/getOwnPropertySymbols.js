


if (typeof Symbol === "function") {
    assertDeepEq(Object.getOwnPropertySymbols({}), []);

    
    assertEq(Object.getOwnPropertySymbols({a: 1, b: 2}).length, 0);
    assertEq(Object.getOwnPropertySymbols([0, 1, 2, 3]).length, 0);

    
    var iterable = {};
    Object.defineProperty(iterable, Symbol.iterator, {
        value: () => [][Symbol.iterator]()
    });
    assertDeepEq(Object.getOwnPropertySymbols(iterable), [Symbol.iterator]);
    assertDeepEq(Object.getOwnPropertySymbols(new Proxy(iterable, {})), [Symbol.iterator]);

    
    var obj = {};
    for (var i = 0; i < 1000; i++) {
        obj[Symbol.for("x" + i)] = 1;
    }
    assertEq(Object.getOwnPropertyNames(obj).length, 0);
    var symbols = Object.getOwnPropertySymbols(obj);
    assertEq(symbols.length, 1000);
    assertEq(symbols.indexOf(Symbol.for("x0")) !== -1, true);
    assertEq(symbols.indexOf(Symbol.for("x241")) !== -1, true);
    assertEq(symbols.indexOf(Symbol.for("x999")) !== -1, true);
    assertEq(Object.getOwnPropertySymbols(new Proxy(obj, {})).length, 1000);

    
    assertEq(Object.getOwnPropertySymbols(Object.create(obj)).length, 0);
    assertEq(Object.getOwnPropertySymbols(new Proxy(Object.create(obj), {})).length, 0);

    
    
    assertThrowsInstanceOf(() => Object.getOwnPropertySymbols(), TypeError);
    assertThrowsInstanceOf(() => Object.getOwnPropertySymbols(undefined), TypeError);
    assertThrowsInstanceOf(() => Object.getOwnPropertySymbols(null), TypeError);
    for (var primitive of [true, 1, 3.14, "hello", Symbol()])
        assertEq(Object.getOwnPropertySymbols(primitive).length, 0);

    assertEq(Object.getOwnPropertySymbols.length, 1);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
