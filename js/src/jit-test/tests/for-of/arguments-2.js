

function f() {
    return arguments;
}

var s = '';
var args = f('a', 'b', 'c');
for (var v of args)
    s += v;
assertEq(s, 'abc');
