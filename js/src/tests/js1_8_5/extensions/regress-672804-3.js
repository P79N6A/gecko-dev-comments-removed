


var e = [], x = {b: []};
function f() {
    let (a = [[] for (x in e)], {b: []} = x) {}
}
trap(f, 3, '');
f();

reportCompare(0, 0, 'ok');
