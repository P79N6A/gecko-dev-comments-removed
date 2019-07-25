setDebug(true);

function nop(){}
function caller(obj) {
  assertJit();
  var x = ({ dana : "zuul" });
  return x;
}

var pc = line2pc(caller, pc2line(caller, 0) + 2);
trap(caller, pc, "x = 'success'; nop()");
assertEq(caller(this), "success");
