
var target = function (x, y) {
    return x + y;
}
assertEq(new Proxy(target, {})(2, 3), 5);
