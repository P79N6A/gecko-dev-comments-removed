



var x = {};
for (var i = 0; i < 2; i++) {
    Object.defineProperty(x, "y", {configurable: true, value: function () {}});
    x.y();
}
reportCompare(0, 0, "ok");
