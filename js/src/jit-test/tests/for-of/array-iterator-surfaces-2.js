

load(libdir + "iteration.js");

var proto = Object.getPrototypeOf([][std_iterator]());
assertEq(Object.getPrototypeOf(proto), Iterator.prototype);

function check(it) {
    assertEq(typeof it, 'object');
    assertEq(Object.getPrototypeOf(it), proto);
    assertEq(Object.getOwnPropertyNames(it).length, 0);
    assertEq(it[std_iterator](), it);

    
    it.x = 0;
    var s = '';
    for (var p in it)
        s += p + '.';
    assertEq(s, 'x.');
}

check([][std_iterator]());
check(Array.prototype[std_iterator].call({}));
