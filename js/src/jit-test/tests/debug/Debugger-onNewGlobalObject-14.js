

var dbg = new Debugger;
var log = '';
dbg.onNewGlobalObject = function (global) {
  log += 'n';
  var gw = dbg.addDebuggee(global);
  gw.defineProperty('x', { value: -1 });
  
  assertEq(gw.evalInGlobalWithBindings('Math.atan2(y,x)', { y: 0 }).return, Math.PI);
  
  assertEq(gw.evalInGlobalWithBindings('x.toString()', { x: gw }).return, "[object global]");
};

newGlobal();

assertEq(log, 'n');
