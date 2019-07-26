

function f() {
    return (arguments for (x of [1]));
}

var args = f("ponies").next();
assertEq(args[0], "ponies");
