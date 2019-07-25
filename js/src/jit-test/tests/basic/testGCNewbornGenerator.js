
try {
    function f() {}
    (1 for (x in []))
} catch (e) {}
gc()
