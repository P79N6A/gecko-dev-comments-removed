






var summary = "Partial flat expression closure upvar order test";

function f(a) {
    if (a) {
        let b = 42;
        let c = function () b+a;
        ++b;
        return c;
    }
    return null;
}

var expect = 44;
var actual = f(1)();

reportCompare(expect, actual, summary);
