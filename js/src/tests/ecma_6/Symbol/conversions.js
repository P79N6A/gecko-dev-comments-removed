

if (typeof Symbol === "function") {
    var symbols = [
        Symbol(),
        Symbol("one"),
        Symbol.for("two"),
        Symbol.iterator
    ];

    if (Symbol.toPrimitive in Symbol.prototype) {
        
        
        
        throw new Error("Congratulations on implementing @@toPrimitive! Please update this test.");
    }

    for (var sym of symbols) {
        
        var symobj = Object(sym);
        assertThrowsInstanceOf(() => Number(symobj), TypeError);
        assertThrowsInstanceOf(() => String(symobj), TypeError);
        assertThrowsInstanceOf(() => symobj < 0, TypeError);
        assertThrowsInstanceOf(() => 0 < symobj, TypeError);
        assertThrowsInstanceOf(() => symobj + 1, TypeError);
        assertThrowsInstanceOf(() => "" + symobj, TypeError);
        assertEq(sym == symobj, true);
        assertEq(sym === symobj, false);
        assertEq(symobj == 0, false);
        assertEq(0 != symobj, true);

        
        assertEq(Boolean(sym), true);
        assertEq(!sym, false);
        assertEq(sym || 13, sym);
        assertEq(sym && 13, 13);

        
        assertThrowsInstanceOf(() => +sym, TypeError);
        assertThrowsInstanceOf(() => sym | 0, TypeError);

        
        assertThrowsInstanceOf(() => "" + sym, TypeError);
        assertThrowsInstanceOf(() => sym + "", TypeError);
        assertThrowsInstanceOf(() => "" + [1, 2, Symbol()], TypeError);
        assertThrowsInstanceOf(() => ["simple", "thimble", Symbol()].join(), TypeError);

        
        assertEq(String(sym), sym.toString());
        assertThrowsInstanceOf(() => String(Object(sym)), TypeError);

        
        assertThrowsInstanceOf(() => new String(sym), TypeError);

        
        var obj = Object(sym);
        assertEq(typeof obj, "object");
        assertEq(Object.prototype.toString.call(obj), "[object Symbol]");
        assertEq(Object.getPrototypeOf(obj), Symbol.prototype);
        assertEq(Object.getOwnPropertyNames(obj).length, 0);
        assertEq(Object(sym) === Object(sym), false);  
        var f = function () { return this; };
        assertEq(f.call(sym) === f.call(sym), false);  
    }
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
