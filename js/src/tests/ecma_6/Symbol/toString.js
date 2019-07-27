


if (typeof Symbol === "function") {
    var cases = [
        {sym: Symbol(), str: "Symbol()"},
        {sym: Symbol("ok"), str: "Symbol(ok)"},
        {sym: Symbol("\0"), str: "Symbol(\0)"},
        {sym: Symbol.iterator, str: "Symbol(Symbol.iterator)"},
        {sym: Symbol.for("dummies"), str: "Symbol(dummies)"}
    ];

    
    
    for (var test of cases) {
        assertEq(test.sym.toString(), test.str);
        assertEq(Object(test.sym).toString(), test.str);
    }

    
    var nonsymbols = [
        undefined, null, "not-ok", new String("still-not-ok"), {}, []
    ];
    for (var nonsym of nonsymbols)
        assertThrowsInstanceOf(() => Symbol.prototype.toString.call(nonsym), TypeError);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
