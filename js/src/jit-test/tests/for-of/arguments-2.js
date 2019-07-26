

function f() {
    return arguments;
}

var s = '';
var args = f('a', 'b', 'c');
Object.prototype.iterator = Array.prototype.iterator;
for (var v of args)
    s += v;
assertEq(s, 'abc');
