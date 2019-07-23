





function f(e) {
    eval("[function () { w.r = 0 }() for (w in [0])]")
}
f(0);
reportCompare(0, 0, "");
