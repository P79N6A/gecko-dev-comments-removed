
if (typeof Symbol === "function") {
    g = (function() {
        var Int32ArrayView = Int32Array();
        function f() {
            Int32ArrayView[Symbol() >> 2]
        }
        return f;
    })();
    try {
        g();
    } catch (e) {}
    g();
} else {
    throw new TypeError("pass");
}
