

var s;
function f() {
    arguments.length = 2;
    for (var v of arguments)
        s += v;
}

s = '';
f('a', 'b', 'c', 'd', 'e');
assertEq(s, 'ab');
