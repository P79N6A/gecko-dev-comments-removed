

load(libdir + "iteration.js");



Object.prototype[Symbol.iterator] = Array.prototype[Symbol.iterator];

var s;
function test() {
    for (var v of arguments)
        s += v;
}

s = '';
test();
assertEq(s, '');

s = '';
test('x', 'y');
assertEq(s, 'xy');
