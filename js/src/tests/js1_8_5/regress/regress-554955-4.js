





function f(s) {
    eval(s);
    return function(a) {
        eval(a);
        let (c = 3) {
            eval(s);
            return b;
        };
    };
}

var b = 1;
var g1 = f('');
var g2 = f('');


g1('');





assertEq(g2('var b=2'), 2);

reportCompare(true, true);
