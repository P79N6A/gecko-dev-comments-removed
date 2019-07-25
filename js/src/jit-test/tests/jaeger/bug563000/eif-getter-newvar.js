
setDebug(true);

this.__defineGetter__("someProperty", function () { evalInFrame(1, "var x = 'success'"); });
function caller(code, obj) {
  assertJit();
  eval(code); 
  obj.someProperty;
  return x;
}
assertEq(caller("var y = 'ignominy'", this), "success");
