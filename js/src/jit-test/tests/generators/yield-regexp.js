

load(libdir + "asserts.js");
















function f1() {
  yield /abc/g;
}

var g = f1();
var v;
v = g.next();
assertEq(v instanceof RegExp, true);
assertEq(v.toString(), "/abc/g");
assertThrowsValue(() => g.next(), StopIteration);

function* f2() {
  yield /abc/g;
}

g = f2();
v = g.next();
assertEq(v.done, false);
assertEq(v.value instanceof RegExp, true);
assertEq(v.value.toString(), "/abc/g");
v = g.next();
assertEq(v.done, true);
