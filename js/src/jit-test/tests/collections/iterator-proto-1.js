


function test(obj0, obj1) {
    var iter0 = obj0.iterator(), iter1 = obj1.iterator();
    var proto = Object.getPrototypeOf(iter0);
    assertEq(Object.getPrototypeOf(iter1), proto);
    assertEq(Object.getPrototypeOf(proto), Iterator.prototype);
}

test([], [1]);
test(Map(), Map([[1, 1]]));
test(Set(), Set([1]));
