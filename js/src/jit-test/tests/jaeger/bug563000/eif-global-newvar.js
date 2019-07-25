
setDebug(true);

function callee() {
  assertJit();
  evalInFrame(1, "var x = 'success'");
}
callee();
assertEq(x, "success");
