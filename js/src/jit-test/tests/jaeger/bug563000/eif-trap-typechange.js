
setDebug(true);

function nop(){}
function caller(obj) {
  var x = ({ dana : "zuul" });
  return x;
}

var pc = line2pc(caller, pc2line(caller, 0) + 1);
trap(caller, pc, "x = 'success'; nop()");
assertEq(caller(this), "success");
