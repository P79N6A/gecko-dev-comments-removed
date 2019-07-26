

load(libdir + "iteration.js");



Object.prototype[std_iterator] = Array.prototype[std_iterator];

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
