
setDebug(true);

function nop(){}
function caller(code, obj) {
  eval(code); 
  return x;
}
trap(caller, 32, "var x = 'success'; nop()");
assertEq(caller("var y = 'ignominy'", this), "success");
