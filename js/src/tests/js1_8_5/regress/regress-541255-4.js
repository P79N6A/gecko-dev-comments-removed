





function f(e) {
    eval("[function () { w.r = 0 }() for (w in [0,1,2,3,4,5,6,7,8,9])]")
}
f(0);
reportCompare(0, 0, "");
