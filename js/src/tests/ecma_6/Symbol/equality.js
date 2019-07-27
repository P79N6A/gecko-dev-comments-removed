


if (typeof Symbol === "function") {
    
    assertEq(Symbol.for("q") === Symbol.for("q"), true);

    
    var symbols = [
        Symbol(),
        Symbol("Symbol.iterator"),
        Symbol("Symbol.iterator"),  
        Symbol.for("Symbol.iterator"),
        Symbol.iterator
    ];

    
    
    for (var i = 0; i < symbols.length; i++) {
        for (var j = i; j < symbols.length; j++) {
            var expected = (i === j);
            assertEq(symbols[i] == symbols[j], expected);
            assertEq(symbols[i] != symbols[j], !expected);
            assertEq(symbols[i] === symbols[j], expected);
            assertEq(symbols[i] !== symbols[j], !expected);
            assertEq(Object.is(symbols[i], symbols[j]), expected);
        }
    }
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
