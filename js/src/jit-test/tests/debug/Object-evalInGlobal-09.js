

(function () {
  var g = newGlobal();
  var dbg = new Debugger;
  var gw = dbg.addDebuggee(g);
  gw.evalInGlobalWithBindings("eval('Math')",{}).return
})();

