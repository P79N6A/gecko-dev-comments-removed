


if (typeof Symbol === "function") {
    
    assertEq(typeof Symbol.iterator, "symbol");

    
    assertEq(Symbol.iterator !== Symbol.for("Symbol.iterator"), true);

    
    if (typeof Realm === 'function')
        throw new Error("please update this test to use Realms");
    if (typeof newGlobal === 'function') {
        var g = newGlobal();
        assertEq(Symbol.iterator, g.Symbol.iterator);
    }
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
