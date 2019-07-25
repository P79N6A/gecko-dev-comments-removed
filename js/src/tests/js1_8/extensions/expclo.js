






var summary = "Flat expression closure source coordinate fencepost test";

function f(a) {
    if (a) {
        let b = 42;
        let c = function () a+b;
        ++b;
        return c;
    }
    return null;
}

var expect = 44;
var actual = f(1)();

reportCompare(expect, actual, summary);
