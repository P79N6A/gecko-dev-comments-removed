const constructors = [
    Int8Array,
    Uint8Array,
    Uint8ClampedArray,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Float32Array,
    Float64Array
];

for (var constructor of constructors) {
    if (typeof newGlobal !== 'function')
        break;

    
    
    var g = newGlobal();
    var ga = g[constructor.name].from([1, 2, 3]);
    assertEq(ga instanceof g[constructor.name], true);

    
    var p = constructor.from.call(g[constructor.name], [1, 2, 3]);
    assertEq(p instanceof g[constructor.name], true);
    var q = g[constructor.name].from.call(constructor, [3, 4, 5]);
    assertEq(q instanceof constructor, true);

    
    
    var h = newGlobal(), result = undefined;
    h.mainGlobal = this;
    h.eval("function f() { mainGlobal.result = this; }");
    g[constructor.name].from.call(constructor, [5, 6, 7], h.f);
    
    
    this.globalName = "main";
    g.globalName = "g";
    h.globalName = "h";
    assertEq(result.globalName, "h");
}

if (typeof reportCompare === "function")
    reportCompare(true, true);
