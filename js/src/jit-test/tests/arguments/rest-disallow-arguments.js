load(libdir + "asserts.js");
var ieval = eval;


assertThrowsInstanceOf(function () {
    ieval("function x(...rest) { arguments; }");
}, SyntaxError)
assertThrowsInstanceOf(function () {
    Function("...rest", "arguments;");
}, SyntaxError);
assertThrowsInstanceOf(function (...rest) {
    eval("arguments;");
}, SyntaxError);
assertThrowsInstanceOf(function (...rest) {
    eval("arguments = 42;");
}, SyntaxError);

function g(...rest) {
    assertThrowsInstanceOf(h, Error);
}
function h() {
    g.arguments;
}
g();


function still_use_eval(...rest) {
    eval("x = 4");
}
still_use_eval();