

load(libdir + "referencesVia.js");

var m = new Map;
for (var i = 0; i < 20; i++) {
    var n = new Map;
    n.set(m, i);
    assertEq(referencesVia(n, 'key', m), true);
    m = n;
}

gc();
gc();


