







function test(makeNonArray) {
    function C() {}
    C.prototype = []
    if (makeNonArray)
        C.prototype.constructor = C
    c = new C();
    c.push("foo");
    return c.length
}
assertEq(test(true), 1);
assertEq(test(false), 1);


var a = [];
a.slowify = 1;
var b = Object.create(a);
b.length = 12;
assertEq(b.length, 12);


var b = Object.create(Array.prototype);
b.length = 12;
assertEq(b.length, 12);

reportCompare(true, true);
