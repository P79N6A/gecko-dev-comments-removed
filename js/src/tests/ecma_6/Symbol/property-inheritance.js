


function F() {}

if (typeof Symbol === "function") {
    var sym = Symbol.for("hello");
    var f = new F();

    
    F.prototype[sym] = "world";
    assertEq(sym in f, true);
    assertEq(f.hasOwnProperty(sym), false);
    assertEq(f[sym], "world");

    
    f[sym] = "kitty";
    assertEq(f[sym], "kitty");
    assertEq(F.prototype[sym], "world");

    
    assertEq(delete f[sym], true);
    assertEq(f.hasOwnProperty(sym), false);
    assertEq(f[sym], "world");

    
    var value = undefined;
    Object.defineProperty(F.prototype, sym, {
        configurable: true,
        get: function () { return 23; },
        set: function (v) { value = v; }
    });
    assertEq(sym in f, true);
    assertEq(f.hasOwnProperty(sym), false);
    assertEq(f[sym], 23);
    f[sym] = "gravity";
    assertEq(value, "gravity");

    
    Object.defineProperty(F.prototype, sym, {
        set: undefined
    });
    assertThrowsInstanceOf(function () { "use strict"; f[sym] = 0; }, TypeError);

    
    var g = Object.create(f);
    for (var i = 0; i < 100; i++)
        g = Object.create(g);
    assertEq(g[sym], 23);
}

if (typeof reportCompare === "function")
    reportCompare(0, 0);
