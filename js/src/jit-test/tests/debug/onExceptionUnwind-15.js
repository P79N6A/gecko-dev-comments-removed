


g = newGlobal();
g.parent = this;
g.eval("Debugger(parent).onExceptionUnwind=(function() {})");
function throwInNext() {
  yield 1;
  yield 2;
  yield 3;
  throw 42;
}

function f() {
  for (var o of new throwInNext);
}

var log = "";
try {
  f();
} catch (e) {
  log += e;
}

assertEq(log, "42");
