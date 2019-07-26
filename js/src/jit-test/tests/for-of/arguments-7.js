

Object.prototype.iterator = Array.prototype.iterator;

var s;
function f() {
    for (var v of arguments) {
        s += v;
        arguments.length--;
    }
}

s = '';
f('a', 'b', 'c', 'd', 'e');
assertEq(s, 'abc');
