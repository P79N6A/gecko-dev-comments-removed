
setDebug(true);

function nop(){}
function caller(obj) {
  var x = "failure";
  return x;
}
trap(caller, 9 , "x = 'success'; nop()");
assertEq(caller(this), "success");
