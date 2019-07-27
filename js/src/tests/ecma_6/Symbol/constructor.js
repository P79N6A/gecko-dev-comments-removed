


if (typeof Symbol === "function") {
    
    var sym = Symbol();
    assertThrowsInstanceOf(() => Symbol(sym), TypeError);

    
    assertEq(Symbol(undefined).toString(), "Symbol()");

    
    assertEq(Symbol(7).toString(), "Symbol(7)");
    assertEq(Symbol(true).toString(), "Symbol(true)");
    assertEq(Symbol(null).toString(), "Symbol(null)");
    assertEq(Symbol([1, 2]).toString(), "Symbol(1,2)");
    var symobj = Object(sym);
    assertThrowsInstanceOf(() => Symbol(symobj), TypeError);

    var hits = 0;
    var obj = {
        toString: function () {
            hits++;
            return "ponies";
        }
    };
    assertEq(Symbol(obj).toString(), "Symbol(ponies)");
    assertEq(hits, 1);

    assertEq(Object.getPrototypeOf(Symbol.prototype), Object.prototype);

    
    assertThrowsInstanceOf(() => Symbol.prototype.valueOf(), TypeError);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
