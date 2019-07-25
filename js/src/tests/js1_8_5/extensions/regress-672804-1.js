


var a = 0;
function f() {
    let (a = let (x = 1) x) {}
}

trap(f, 4, 'assertEq(evalInFrame(1, "a"), 0)');
f();

reportCompare(0, 0, 'ok');
