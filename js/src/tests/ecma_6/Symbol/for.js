


if (typeof Symbol === "function") {
    
    assertEq(Symbol.for("ponies"), Symbol.for("ponies"));

    
    var one = Array(64+1).join("x");
    var two = Array(8+1).join(Array(8+1).join("x"));
    assertEq(Symbol.for(one), Symbol.for(two));

    
    var sym = Symbol("123");
    assertEq(Symbol.for("123") !== sym, true);

    
    assertEq(typeof Symbol.for(""), "symbol");

    
    assertEq(Symbol.for(3), Symbol.for("3"));
    assertEq(Symbol.for(null), Symbol.for("null"));
    assertEq(Symbol.for(undefined), Symbol.for("undefined"));
    assertEq(Symbol.for(), Symbol.for("undefined"));

    
    var foo = Symbol.for("foo");
    assertEq(Symbol.for.call(String, "foo"), foo);
    assertEq(Symbol.for.call(3.14, "foo"), foo);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
