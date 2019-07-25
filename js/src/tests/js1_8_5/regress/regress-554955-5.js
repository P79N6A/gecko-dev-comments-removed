





function f(s) {
    if (s) {
        function b() { }
    }
    return function(a) {
        eval(a);
        return b;
    };
}

var b = 1;
var g1 = f(false);
var g2 = f(true);


g1('');





assertEq(typeof g2(''), "function");

reportCompare(true, true);
