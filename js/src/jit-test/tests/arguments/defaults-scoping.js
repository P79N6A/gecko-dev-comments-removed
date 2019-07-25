var x = 'global';
function f(a=x) {  
    var x = 'local';
    return a;
}
assertEq(f(), undefined);

function g(f=function () { return ++x; }) {  
    var x = 0;
    return f;
}
var gf = g();
assertEq(gf(), 1);
assertEq(gf(), 2);
gf = g();
assertEq(gf(), 1);

function h(f=function (s) { return eval(s); }) {  
    var x = 'hlocal';
    return f;
}
var hf = h();
assertEq(hf('x'), 'hlocal');
assertEq(hf('f'), hf);
assertEq(hf('var x = 3; x'), 3);

function j(expr, v=eval(expr)) {
    return v;
}
assertEq(j("expr"), "expr");
assertEq(j("v"), undefined);
assertEq(j("Array"), Array);
assertEq(j("arguments").length, 1);
