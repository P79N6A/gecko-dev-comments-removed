

var g = newGlobal();
g.parent = this;
g.eval("new Debugger(parent).onExceptionUnwind = function () { };");
enableSPSProfiling();

try {
  
  enableSingleStepProfiling();
} catch (e) {
  throw new ReferenceError;
}

enableSingleStepProfiling();
a()
