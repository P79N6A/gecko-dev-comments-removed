
setDebug(true);

function nop(){}
function caller(code, obj) {
  assertJit();
  eval(code); 
  return x;
}
trap(caller, 7, "var x = 'success'; nop()");
assertEq(caller("var y = 'ignominy'", this), "success");
