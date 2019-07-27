

if (typeof Symbol === "function") {
    
    var obj = {};
    var x = Symbol.for("x");
    obj[x] = 0;
    var hits = 0;
    obj.watch(x, function (id, oldval, newval) { hits++; return newval; });
    for (var i = 0; i < 10; i++)
        obj[x] = i;
    assertEq(hits, 10);

    
    hits = 0;
    for (var i = 0; i < 10; i++)
        obj[x]++;
    assertEq(hits, 10);

    
    hits = 0;
    obj = {};
    obj.watch(x, function (id, oldval, newval) { hits++; return newval; });
    for (var i = 0; i < 10; i++) {
        obj[x] = 1;
        delete obj[x];
    }
    assertEq(hits, 10);
}
