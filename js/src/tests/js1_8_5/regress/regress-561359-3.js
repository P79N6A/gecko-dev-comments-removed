



function f(s) {
    with (s)
        return {m: function () { return a; }};
}
var obj = f({a: 'right'});
var a = 'wrong';
assertEq(obj.m(), 'right');

reportCompare(0, 0, 'ok');
