



assertDeepEq(Array.from([3, 4, 5], undefined), [3, 4, 5]);
assertDeepEq(Array.from([4, 5, 6], undefined, Math), [4, 5, 6]);


var log = [];
function f() {
    log.push(Array.from(arguments));
    return log.length;
}
assertDeepEq(Array.from(['a', 'e', 'i', 'o', 'u'], f), [1, 2, 3, 4, 5]);
assertDeepEq(log, [['a', 0], ['e', 1], ['i', 2], ['o', 3], ['u', 4]]);



log = [];
assertDeepEq(Array.from({0: "zero", 1: "one", length: 2}, f), [1, 2]);
assertDeepEq(log, [["zero", 0], ["one", 1]]);



log = [];
function C() {}
C.from = Array.from;
var c = new C;
c[0] = 1;
c[1] = 2;
c.length = 2;
assertDeepEq(C.from(["zero", "one"], f), c);
assertDeepEq(log, [["zero", 0], ["one", 1]]);


assertDeepEq(Array.from([0, 1, , 3], String), ["0", "1", "undefined", "3"]);
var arraylike = {length: 4, "0": 0, "1": 1, "3": 3};
assertDeepEq(Array.from(arraylike, String), ["0", "1", "undefined", "3"]);

if (typeof reportCompare === 'function')
    reportCompare(0, 0);
