


var g1 = newGlobal();
var g2 = newGlobal();
var g3 = newGlobal();

g1.eval("" + function f() {
  var name = "f";
  g();
  return name;
});
g2.eval("" + function g() {
  var name = "g";
  h();
  return name;
});
g3.eval("" + function h() {
  var name = "h";
  toggle();
  return name;
});

g1.g = g2.g;
g2.h = g3.h;

function name(f) {
  return f.environment.getVariable("name");
}

var dbg = new Debugger;
g3.toggle = function () {
  var frame;

  
  dbg.addDebuggee(g1);
  frame = dbg.getNewestFrame();
  assertEq(name(frame), "f");

  
  dbg.addDebuggee(g3);
  frame = dbg.getNewestFrame();
  assertEq(name(frame), "h");
  assertEq(name(frame.older), "f");

  
  dbg.addDebuggee(g2);
  frame = dbg.getNewestFrame();
  assertEq(name(frame), "h");
  assertEq(name(frame.older), "g");
  assertEq(name(frame.older.older), "f");
};

g1.eval("(" + function () { f(); } + ")();");

