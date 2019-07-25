

load(libdir + "referencesVia.js");

var s = new Set;
for (var i = 0; i < 20; i++) {
    var t = new Set;
    t.add(s);
    assertEq(referencesVia(t, 'key', s), true);
    s = t;
}

gc();
gc();


