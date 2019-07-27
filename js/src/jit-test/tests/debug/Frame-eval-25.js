




load(libdir + "evalInFrame.js");

function h() {
  evalInFrame(1, "a.push(0)");
}

function f() {
  var a = arguments;
  h();
}

f();
