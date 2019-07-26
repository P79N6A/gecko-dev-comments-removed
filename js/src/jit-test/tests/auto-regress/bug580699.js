


function f() {
    (function () {
        x;
        function a() {}
        print(a)
    })()
}
__defineGetter__("x", gc)
f()
