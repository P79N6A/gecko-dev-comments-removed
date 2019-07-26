

function f() {
    return s => eval(s);
}

var g = f();
var args = g("arguments");
assertEq(typeof args, "object");
assertEq(args !== g("arguments"), true);
assertEq(args.length, 1);
assertEq(args[0], "arguments");

