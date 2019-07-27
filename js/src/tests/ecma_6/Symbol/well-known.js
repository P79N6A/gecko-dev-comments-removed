


var names = [
    "iterator",
    "match",
];

for (var name of names) {
    
    assertEq(typeof Symbol[name], "symbol");

    
    assertEq(Symbol[name] !== Symbol.for("Symbol." + name), true);

    
    if (typeof Realm === 'function')
        throw new Error("please update this test to use Realms");
    if (typeof newGlobal === 'function') {
        var g = newGlobal();
        assertEq(Symbol[name], g.Symbol[name]);
    }

    
    var desc = Object.getOwnPropertyDescriptor(Symbol, name);
    assertEq(typeof desc.value, "symbol");
    assertEq(desc.writable, false);
    assertEq(desc.enumerable, false);
    assertEq(desc.configurable, false);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
