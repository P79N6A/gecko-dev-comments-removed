
load(libdir + 'asserts.js');

var g = newGlobal({ invisibleToDebugger: true });

assertThrowsInstanceOf(() => { new Debugger(g); }, TypeError);
