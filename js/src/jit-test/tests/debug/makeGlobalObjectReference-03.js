
load(libdir + 'asserts.js');

var g = newGlobal({ invisibleToDebugger: true });

assertThrowsInstanceOf(function () {
  (new Debugger).makeGlobalObjectReference(g)
}, TypeError);
