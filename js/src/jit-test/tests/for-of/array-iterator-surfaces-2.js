

var proto = Object.getPrototypeOf([].iterator());
assertEq(Object.getPrototypeOf(proto), Iterator.prototype);

function check(it) {
    assertEq(typeof it, 'object');
    assertEq(Object.getPrototypeOf(it), proto);
    assertEq(Object.getOwnPropertyNames(it).length, 0);
    assertEq(it.iterator(), it);

    
    it.x = 0;
    var s = '';
    for (var p in it)
        s += p + '.';
    assertEq(s, 'x.');
}

check([].iterator());
check(Array.prototype.iterator.call({}));
check(Array.prototype.iterator.call(undefined));
