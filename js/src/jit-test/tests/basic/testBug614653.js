
for (var i = 0; i < 100; ++i) {
    var arr = [];
    var s = "abcdefghijklmnop";
    for (var i = 0; i < 50000; ++i) {
        s = "<" + s + ">";
        arr.push(s);
    }
    gc();
}
