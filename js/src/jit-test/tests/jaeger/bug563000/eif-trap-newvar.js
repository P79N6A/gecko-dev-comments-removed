
setDebug(true);

function nop(){}
function caller(obj) {
  var x = 'ignominy';
  return x;
}
trap(caller, 9 , "var x = 'success'; nop()");
assertEq(caller(this), "success");
