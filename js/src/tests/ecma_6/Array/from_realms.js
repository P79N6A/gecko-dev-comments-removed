


if (typeof newGlobal === 'function') {
    
    
    var g = newGlobal();
    var ga = g.Array.from([1, 2, 3]);
    assertEq(ga instanceof g.Array, true);

    
    var from = g.Array.from
    var ga2 = from([1, 2, 3]);
    assertEq(ga2 instanceof g.Array, true);

    
    var p = Array.from.call(g.Array, [1, 2, 3]);
    assertEq(p instanceof g.Array, true);
    var q = g.Array.from.call(Array, [3, 4, 5]);
    assertEq(q instanceof Array, true);

    
    
    var h = newGlobal(), result = undefined;
    h.mainGlobal = this;
    h.eval("function f() { mainGlobal.result = this; }");
    g.Array.from.call(Array, [5, 6, 7], h.f);
    
    this.name = "main";
    g.name = "g";
    h.name = "h";
    assertEq(result.name, "h");
}

if (typeof reportCompare === 'function')
    reportCompare(0, 0);
